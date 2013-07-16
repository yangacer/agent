#include <iostream>
#include <cstdio>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#endif

#include <boost/filesystem.hpp>
#include "archive.h"
#include "archive_entry.h"

using namespace std;
namespace fs = boost::filesystem;

void usage()
{
  cout << "Usage: archive <output_name> <dir>\n";
  exit(1);
}

int main(int argc, char **argv)
{
  if(argc < 3) usage();

  struct archive *a;
  struct archive_entry *entry;
  char buff[8192];
  int len;
  int fd;
  char const *outname = argv[1];

  if(!fs::is_directory(argv[2])) {
    cerr << argv[2] << " is not a directory.\n";
    usage();
  }

  fs::recursive_directory_iterator iter(argv[2]), end;

  a = archive_write_new();
  archive_write_set_format_pax_restricted(a); // Note 1
  archive_write_add_filter_gzip(a);
  /*
  int r = archive_write_add_filter_by_name(a, "gzip");
  if (r == ARCHIVE_WARN) {
    cout << "[warn] Filter is not suported on this platform\n";
  } else if (r == ARCHIVE_FATAL ) {
    cout << "[fatal] Filter is not suported on this platform\n";
  } else {
    cout << "OK\n";
  }
  */
  archive_write_open_filename(a, outname);
  entry = archive_entry_new();
  while (iter != end) {
    if( fs::is_regular(iter->path())) { 
      string filename = iter->path().string();
	  struct stat sb;
	  cout << filename << "\n";
	  stat(filename.c_str(), &sb);
      archive_entry_set_pathname(entry, filename.c_str());
	  archive_entry_copy_stat(entry, &sb);
      //archive_entry_set_size(entry, st.st_size); 
      archive_entry_set_filetype(entry, AE_IFREG);
      archive_entry_set_perm(entry, 0644); 
      archive_write_header(a, entry); 
      fd = open(filename.c_str(), O_RDONLY); 
      len = read(fd, buff, sizeof(buff)); 
      while ( len > 0 ) { 
        archive_write_data(a, buff, len);
        len = read(fd, buff, sizeof(buff));
      } 
      close(fd); 
      archive_entry_clear(entry); 
    } 
    ++iter; 
  }
  archive_write_close(a); // Note 4 archive_write_free(a); // Note 5 
  archive_write_free(a);
}
