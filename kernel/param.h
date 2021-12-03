#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       32  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         15  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGDEPTH     4   // Number of indivual log entries
#define LOGSIZE      (MAXOPBLOCKS*3)*LOGDEPTH  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)*LOGDEPTH  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
