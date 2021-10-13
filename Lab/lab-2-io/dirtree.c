//--------------------------------------------------------------------------------------------------
// System Programming                         I/O Lab                                    Fall 2021
//
/// @file
/// @brief resursively traverse directory tree and list all entries
/// @author lucetre
/// @studid 2016-19516
//--------------------------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>

#define MAX_DIR 64            ///< maximum number of directories supported

/// @brief output control flags
#define F_TREE      0x1       ///< enable tree view
#define F_SUMMARY   0x2       ///< enable summary
#define F_VERBOSE   0x4       ///< turn on verbose mode

/// @brief struct holding the summary
struct summary {
  unsigned int dirs;          ///< number of directories encountered
  unsigned int files;         ///< number of files
  unsigned int links;         ///< number of links
  unsigned int fifos;         ///< number of pipes
  unsigned int socks;         ///< number of sockets

  unsigned long long size;    ///< total size (in bytes)
  unsigned long long blocks;  ///< total number of blocks (512 byte blocks)
};


/// @brief abort the program with EXIT_FAILURE and an optional error message
///
/// @param msg optional error message or NULL
void panic(const char *msg)
{
  if (msg) fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}


/// @brief read next directory entry from open directory 'dir'. Ignores '.' and '..' entries
///
/// @param dir open DIR* stream
/// @retval entry on success
/// @retval NULL on error or if there are no more entries
struct dirent *getNext(DIR *dir)
{
  struct dirent *next;
  int ignore;

  do {
    errno = 0;
    next = readdir(dir);
    if (errno != 0) {
      return NULL;
    }
    ignore = next && ((strcmp(next->d_name, ".") == 0) || (strcmp(next->d_name, "..") == 0));
  } while (next && ignore);

  return next;
}


/// @brief qsort comparator to sort directory entries. Sorted by name, directories first.
///
/// @param a pointer to first entry
/// @param b pointer to second entry
/// @retval -1 if a<b
/// @retval 0  if a==b
/// @retval 1  if a>b
static int dirent_compare(const void *a, const void *b)
{
  struct dirent *e1 = (struct dirent*)a;
  struct dirent *e2 = (struct dirent*)b;

  // if one of the entries is a directory, it comes first
  if (e1->d_type != e2->d_type) {
    if (e1->d_type == DT_DIR) return -1;
    if (e2->d_type == DT_DIR) return 1;
  }

  // otherwise sorty by name
  return strcmp(e1->d_name, e2->d_name);
}


/// @brief recursively process directory @a dn and print its tree
///
/// @param dn absolute or relative path string
/// @param pstr prefix string printed in front of each entry
/// @param stats pointer to statistics
/// @param flags output control flags (F_*)
void processDir(const char *dn, const char *pstr, struct summary *stats, unsigned int flags)
{
  DIR *d = opendir(dn);
  // erro handling
  if (errno == EACCES) {
    printf((flags & F_TREE) ? "%s`-%s\n" : "%s  %s\n", pstr, "ERROR: Permission denied");
    return;
  }
  else if (errno == ENOTDIR) {
    printf("  ERROR: Not a directory\n");
    return;
  }
  else if (d == NULL) {
    printf("  ERROR: No such file or directory\n");
    return;
  }
  struct dirent *entry;
  
  // determine number of entries before storing entries in dir for dynamic mem allocation
  unsigned int nent = 0;
  while((entry = getNext(d)) != NULL) {
    nent++;
  }
  struct dirent *entries = malloc(sizeof(struct dirent)*nent);
  
  // store every entries in dirent array
  int i = 0;
  d = opendir(dn);
  while((entry = getNext(d)) != NULL) {
    entries[i++] = *entry;
  }

  // sort in order of dir and filename using dirent comparator
  qsort(entries, nent, sizeof(entries[0]), dirent_compare);

  for (unsigned int pos = 0; pos < nent; pos++) {
    struct dirent *this = &entries[pos];
    
    char *fn, *out, *tmp;
    // handling path string
    if (asprintf(&fn, "%s/%s", dn, this->d_name) == -1) panic("OUT OF MEMORY!");
    
    // handling output prefix format string
    if (flags & F_TREE) {
      if (pos == nent - 1) {
        if (asprintf(&out, "%s", pstr) == -1) panic("OUT OF MEMORY!");
        if (asprintf(&tmp, "%s`-%s", out, this->d_name) == -1) panic("OUT OF MEMORY!");
        if (asprintf(&out, "%s  ", out) == -1) panic("OUT OF MEMORY!");
      } else {
        if (asprintf(&out, "%s|", pstr) == -1) panic("OUT OF MEMORY!");
        if (asprintf(&tmp, "%s-%s", out, this->d_name) == -1) panic("OUT OF MEMORY!");
        if (asprintf(&out, "%s ", out) == -1) panic("OUT OF MEMORY!");
      }
    } else {
      if (asprintf(&out, "%s  ", pstr) == -1) panic("OUT OF MEMORY!");
      if (asprintf(&tmp, "%s%s", out, this->d_name) == -1) panic("OUT OF MEMORY!");
    }
     
    if (flags & F_VERBOSE) {
      struct stat st;
      if (lstat(fn, &st) == -1) panic("  ERROR: No such file or directory");
      
      if (strlen(tmp) > 54) {
        strncpy(tmp + 51, "...\0", 4);
      }
      printf("%-54s  ", tmp);
      
      unsigned char type = (this->d_type == DT_REG)  ? ' ' : 
                           (this->d_type == DT_DIR)  ? 'd' : 
                           (this->d_type == DT_LNK)  ? 'l' : 
                           (this->d_type == DT_CHR)  ? 'c' : 
                           (this->d_type == DT_BLK)  ? 'b' : 
                           (this->d_type == DT_FIFO) ? 'f' : 
                           (this->d_type == DT_SOCK) ? 's' : '?';
      
      // if directory type is unknown
      if (type == '?') {
        printf("File type could not be determined\n");
        free(fn);
        free(out);
        free(tmp);
        continue;
      }
      
      // accumulate number of files/dirs/links/pipes/sockets/blocks and size
      stats->dirs   += (type == 'd');
      stats->files  += (type == ' ');
      stats->links  += (type == 'l');
      stats->fifos  += (type == 'f');
      stats->socks  += (type == 's');
      stats->size   += st.st_size;
      stats->blocks += st.st_blocks;
      
      // extract user/group id from stat and find corresponding user/group name
      printf("%8s:%-8s  %10ld  %8ld  %c\n", 
             getpwuid(st.st_uid)->pw_name, 
             getgrgid(st.st_gid)->gr_name, 
             st.st_size, 
             st.st_blocks, 
             type);
      
    } else {
      printf("%s\n", tmp);
    }
    if (this->d_type == DT_DIR) {
      // recurse with relative path, output prefix format string, accumulated stats, flag mode
      processDir(fn, out, stats, flags);
    }
    free(fn);
    free(out);
    free(tmp);
  }
}


/// @brief print program syntax and an optional error message. Aborts the program with EXIT_FAILURE
///
/// @param argv0 command line argument 0 (executable)
/// @param error optional error (format) string (printf format) or NULL
/// @param ... parameter to the error format string
void syntax(const char *argv0, const char *error, ...)
{
  if (error) {
    va_list ap;

    va_start(ap, error);
    vfprintf(stderr, error, ap);
    va_end(ap);

    printf("\n\n");
  }

  assert(argv0 != NULL);

  fprintf(stderr, "Usage %s [-t] [-s] [-v] [-h] [path...]\n"
                  "Gather information about directory trees. If no path is given, the current directory\n"
                  "is analyzed.\n"
                  "\n"
                  "Options:\n"
                  " -t        print the directory tree (default if no other option specified)\n"
                  " -s        print summary of directories (total number of files, total file size, etc)\n"
                  " -v        print detailed information for each file. Turns on tree view.\n"
                  " -h        print this help\n"
                  " path...   list of space-separated paths (max %d). Default is the current directory.\n",
                  basename(argv0), MAX_DIR);

  exit(EXIT_FAILURE);
}


/// @brief program entry point
int main(int argc, char *argv[])
{
  //
  // default directory is the current directory (".")
  //
  const char CURDIR[] = ".";
  const char *directories[MAX_DIR];
  int   ndir = 0;

  struct summary dstat, tstat;
  unsigned int flags = 0;

  //
  // parse arguments
  //
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // format: "-<flag>"
      if      (!strcmp(argv[i], "-t")) flags |= F_TREE;
      else if (!strcmp(argv[i], "-s")) flags |= F_SUMMARY;
      else if (!strcmp(argv[i], "-v")) flags |= F_VERBOSE;
      else if (!strcmp(argv[i], "-h")) syntax(argv[0], NULL);
      else syntax(argv[0], "Unrecognized option '%s'.", argv[i]);
    } else {
      // anything else is recognized as a directory
      if (ndir < MAX_DIR) {
        directories[ndir++] = argv[i];
      } else {
        printf("Warning: maximum number of directories exceeded, ignoring '%s'.\n", argv[i]);
      }
    }
  }

  // if no directory was specified, use the current directory
  if (ndir == 0) directories[ndir++] = CURDIR;

  //
  // process each directory
  //
  memset(&tstat, 0, sizeof(tstat));
  for (int i = 0; i < ndir; i++) {
    memset(&dstat, 0, sizeof(dstat));
    
    if (flags & F_SUMMARY) {
      printf("Name                                                        User:Group           Size    Blocks Type\n");
      printf("----------------------------------------------------------------------------------------------------\n");
    }
    printf("%s\n", directories[i]);
    processDir(directories[i], "", &dstat, flags);
    if (flags & F_SUMMARY) {
      char *summary, *files, *dirs, *links, *fifos, *socks;
      // summary line taking care to output grammatically correct English (singular and plural)
      if (asprintf(&files, (dstat.files == 1) ? "%d file" : "%d files", dstat.files) == -1) panic("OUT OF MEMORY!");
      if (asprintf(&dirs, (dstat.dirs == 1) ? "%d directory" : "%d directories", dstat.dirs) == -1) panic("OUT OF MEMORY!");
      if (asprintf(&links, (dstat.links == 1) ? "%d link" : "%d links", dstat.links) == -1) panic("OUT OF MEMORY!");
      if (asprintf(&fifos, (dstat.fifos == 1) ? "%d pipe" : "%d pipes", dstat.fifos) == -1) panic("OUT OF MEMORY!");
      if (asprintf(&socks, (dstat.socks == 1) ? "%d socket" : "%d sockets", dstat.socks) == -1) panic("OUT OF MEMORY!");
      if (asprintf(&summary, "%s, %s, %s, %s, and %s", files, dirs, links, fifos, socks) == -1) panic("OUT OF MEMORY!");
      printf("----------------------------------------------------------------------------------------------------\n");
      printf("%-68s   %14lld %9lld\n\n", summary, dstat.size, dstat.blocks);
    }
    
    // sum it up from every dstats
    tstat.files  += dstat.files;
    tstat.dirs   += dstat.dirs;
    tstat.links  += dstat.links;
    tstat.fifos  += dstat.fifos;
    tstat.socks  += dstat.socks;
    tstat.size   += dstat.size;
    tstat.blocks += dstat.blocks;
  }

  //
  // print grand total
  //
  if ((flags & F_SUMMARY) && (ndir > 1)) {
    printf("Analyzed %d directories:\n"
           "  total # of files:        %16d\n"
           "  total # of directories:  %16d\n"
           "  total # of links:        %16d\n"
           "  total # of pipes:        %16d\n"
           "  total # of socksets:     %16d\n",
           ndir, tstat.files, tstat.dirs, tstat.links, tstat.fifos, tstat.socks);

    if (flags & F_VERBOSE) {
      printf("  total file size:         %16llu\n"
             "  total # of blocks:       %16llu\n",
             tstat.size, tstat.blocks);
    }
  }

  //
  // that's all, folks
  //
  return EXIT_SUCCESS;
}
