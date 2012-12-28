#if defined(ENABLE_DMP)
#include "dmp.h"
#endif

#include "util.h"
#include "debug.h"
#include "dedupdef.h"
#include "encoder.h"
#include "decoder.h"
#include "hashtable.h"
#include "config.h"
#include "queue.h"

#ifdef PARALLEL
#include <pthread.h>
#endif //PARALLEL

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

config * conf;

static int
keys_equal_fn ( void *key1, void *key2 ){
  return (memcmp(((CacheKey*)key1)->sha1name, ((CacheKey*)key2)->sha1name, SHA1_LEN) == 0);
}



/*--------------------------------------------------------------------------*/
static void
usage(char* prog)
{
  printf("usage: %s [-cusfh] [-w gzip/bzip2/none] [-i file/hostname] [-o file/hostname] [-b block_size_in_KB] [-t number_of_threads]\n",prog);
  printf("-c \t\t\tcompress\n");
  printf("-u \t\t\tuncompress\n");
  printf("-s \t\t\tsend output to the receiver\n");
  printf("-f \t\t\tput output into a file\n");
  printf("-w \t\t\tcompress/uncompress technique: gzip/bzip2/none\n");
  printf("-i file\t\t\tthe input file/src host\n");
  printf("-o file\t\t\tthe output file/dst host\n");
  printf("-b block_size_in_KB\tspecify the block size in KB, default=32KB\n");
  printf("-t number of threads per stage \n");
  printf("-h \t\t\thelp\n");
}
/*--------------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
  printf("PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION)"\n");
#else
  printf("PARSEC Benchmark Suite\n");
#endif //PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
        __parsec_bench_begin(__parsec_dedup);
#endif

  int32 compress = TRUE;
  
  conf = (config *) malloc(sizeof(config));
  conf->b_size = 32 * 1024;
  compress_way = COMPRESS_GZIP;
  strcpy(conf->outfile, "");
  conf->preloading = 0;
  conf->nthreads = 1;

  //parse the args
  int ch;
  opterr = 0;
  optind = 1;
  while (-1 != (ch = getopt(argc, argv, "csufpo:i:b:w:t:h"))) {
    switch (ch) {
    case 'c':
      compress = TRUE;
      strcpy(conf->infile, "test.txt");
      strcpy(conf->outfile, "out.cz");
      break;
    case 'u':
      compress = FALSE;
      strcpy(conf->infile, "out.cz");
      strcpy(conf->outfile, "new.txt");
      break;
    case 'f': 
      conf->method = METHOD_FILE;
      break;
    case 'w':
      if (strcmp(optarg, "gzip") == 0)
        compress_way = COMPRESS_GZIP;
      else if (strcmp(optarg, "bzip2") == 0) 
        compress_way = COMPRESS_BZIP2;
      else compress_way = COMPRESS_NONE;
      break;
    case 'o':
      strcpy(conf->outfile, optarg);
      break;
    case 'i':
      strcpy(conf->infile, optarg);
      break;
    case 'h':
      usage(argv[0]);
      return -1;
    case 'b':
      conf->b_size = atoi(optarg) * 1024;
      break;
    case 'p':
      conf->preloading = 1;
      break;
    case 't':
      conf->nthreads = atoi(optarg);
      break;
    case '?':
      fprintf(stdout, "Unknown option `-%c'.\n", optopt);
      usage(argv[0]);
      return -1;
    }
  }

#ifndef PARALLEL
 if (conf->nthreads != 1){
    printf("Number of threads must be 1 (serial version)\n");
    exit(1);
  }
#endif
  cache = create_hashtable(65536, hash_from_key_fn, keys_equal_fn);
  if(cache == NULL) {
    printf("ERROR: Out of memory\n");
    exit(1);
  }

  if (compress) {
    Encode(conf);
  } else {
    Decode(conf);
  }

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_bench_end();
#endif

  return 0;
}
