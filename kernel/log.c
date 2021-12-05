#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  uint64 pos;
  int n;
  int block[LOGSIZE];
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding; // how many FS sys calls are executing.
  int committing;  // in commit(), please wait.
  int dev;
  struct logheader lh;
};
struct log logs[4];
static void recover_from_log(void);
static void commit();
static int prev = 0;
static int curr = 1;
void
initlog(int dev, struct superblock *sb)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&logs[0].lock, "log0");
  initlock(&logs[1].lock, "log1");
  initlock(&logs[2].lock, "log2");
  initlock(&logs[3].lock, "log3");

  for(int i = 0; i < LOGDEPTH; i++){
    logs[i].start = sb->logstart;
    logs[i].size = sb->nlog;
    logs[i].dev = dev;
  }
  recover_from_log();
}

// Copy committed blocks from log to their home location
static void
install_trans(int recovering, int ind)
{
  int tail;

  for (tail = 0; tail < logs[ind].lh.n; tail++) {
    struct buf *lbuf = bread(logs[ind].dev, logs[ind].start+tail+1); // read log block
    struct buf *dbuf = bread(logs[ind].dev, logs[ind].lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    if(recovering == 0)
      bunpin(dbuf);
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header eg, logs[i]
static void
read_head(int ind)
{
  struct buf *buf = bread(logs[ind].dev, logs[ind].start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  logs[ind].lh.n = lh->n;
  for (i = 0; i < logs[ind].lh.n; i++) {
    logs[ind].lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(int ind)
{
  struct buf *buf = bread(logs[ind].dev, logs[ind].start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = logs[ind].lh.n;
  for (i = 0; i < logs[ind].lh.n; i++) {
    hb->block[i] = logs[ind].lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(void)
{
  read_head(3);
  install_trans(1, 3); // if committed, copy from log to disk
  logs[3].lh.n = 0;
  write_head(3); // clear the log
}

// called at the start of each FS system call.
void
begin_op(void)
{
  int ind;
  __atomic_load(&curr,&ind,__ATOMIC_SEQ_CST);
  acquire(&logs[ind].lock);
  while(1){
    if(ind != curr){
      acquire(&logs[curr].lock);
      release(&logs[ind].lock);
      __atomic_load(&curr,&ind,__ATOMIC_SEQ_CST);
    }
    if(logs[ind].lh.n + (logs[ind].outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      // this op might exhaust log space; wait for commit.
      sleep(&logs[ind], &logs[ind].lock);
    } else {
      logs[ind].outstanding += 1;
      release(&logs[ind].lock);
      break;
    }
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
  int do_commit = 0;
  int ind;
  __atomic_load(&curr,&ind,__ATOMIC_SEQ_CST);

  acquire(&logs[ind].lock);

  logs[ind].outstanding -= 1;
  if(logs[ind].committing)
    panic("logs[ind].committing");
  if(logs[ind].outstanding == 0){
    if(logs[prev].committing){ //check if old log is still commiting
      if(logs[ind].lh.n + MAXOPBLOCKS > LOGSIZE){ // Log is full
        sleep(&logs[prev], &logs[ind].lock);
      } else {
        // Log isn't full, return the empty space to potentially be filled
        wakeup(&logs[ind]);
        release(&logs[ind].lock);
        return;
      }
    }
    //Old log isn't committing, proceed
    
    int next = (ind + 1) % 4;
    __atomic_store(&curr, &next, __ATOMIC_SEQ_CST);
    __atomic_store(&prev, &ind, __ATOMIC_SEQ_CST);
    //printf("update curr = %d prev = %d\n", curr, prev);
    do_commit = 1;  
    logs[ind].committing = 1; 
  } else {
    // begin_op() may be waiting for log space,
    // and decrementing logs[3].outstanding has decreased
    // the amount of reserved space.
    wakeup(&logs[ind]);
  }
  release(&logs[ind].lock);

  if(do_commit){
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    commit(ind);
    acquire(&logs[ind].lock);
    logs[ind].committing = 0;
    wakeup(&logs[ind]);
    release(&logs[ind].lock);
  }
}

// Copy modified blocks from cache to logs[3].
static void
write_log(int ind)
{
  int tail;

  for (tail = 0; tail < logs[ind].lh.n; tail++) {
    struct buf *to = bread(logs[ind].dev, logs[ind].start+tail+1); // log block
    struct buf *from = bread(logs[ind].dev, logs[ind].lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelse(from);
    brelse(to);
  }
}

static void
commit(int ind)
{
  if (logs[ind].lh.n > 0) {
    write_log(ind);     // Write modified blocks from cache to log
    write_head(ind);    // Write header to disk -- the real commit
    install_trans(0, ind); // Now install writes to home locations
    logs[ind].lh.n = 0;
    write_head(ind);    // Erase the transaction from the log
  }
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache by increasing refcnt.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i, ind;

  __atomic_load(&curr,&ind,__ATOMIC_SEQ_CST);
  acquire(&logs[ind].lock);
  if (logs[ind].lh.n >= LOGSIZE || logs[ind].lh.n >= logs[ind].size - 1)
    panic("too big a transaction");
  if (logs[ind].outstanding < 1)
    panic("log_write outside of trans");

  for (i = 0; i < logs[ind].lh.n; i++) {
    if (logs[ind].lh.block[i] == b->blockno)   // log absorption
      break;
  }
  logs[ind].lh.block[i] = b->blockno;
  if (i == logs[ind].lh.n) {  // Add new block to log?
    bpin(b);
    logs[ind].lh.n++;
  }
  release(&logs[ind].lock);
}

