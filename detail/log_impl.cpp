#include "log_impl.hpp"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

logger_impl::logger_impl()
{
  if(!thread_) {
    boost::unique_lock<boost::mutex> lock(mutex_);
    io_service_.reset(new boost::asio::io_service);
    work_.reset(new boost::asio::io_service::work(*io_service_));
    os_.reset(new std::ostream(std::cerr.rdbuf()));
    if(!thread_) {
      thread_.reset(new boost::thread(
          boost::bind(
            &logger_impl::run, this)));
      thread_->detach();
    }
  }
}

void logger_impl::destroy()
{
  if(work_) {
    boost::unique_lock<boost::mutex> work_lock(work_mutex_);
    if(work_) work_.reset();
    if(io_service_.get()) {
      if(thread_.get()) {
        boost::unique_lock<boost::mutex> lock(mutex_);
        // XXX Trick for making queued task to be done
        while(0 != io_service_->run_one());
        assert(0 == io_service_->run_one() && "Pending jobs in logger.");
        // fini ostream
        os_->flush();
        if(file_.size()) 
          delete os_->rdbuf();
        
        os_.reset();
        thread_.reset();
      }
      io_service_.reset();
    }
  }
}

boost::asio::io_service& logger_impl::io_service()
{ return *io_service_; }

void logger_impl::use_file(std::string const &filename, boost::uint32_t max_size)
{
  using std::filebuf;
  using std::ios;

  filebuf* fb = 0;
  
  os_->flush();
  if(file_.size()) 
    delete os_->rdbuf();

  fb = new std::filebuf();
  fb->open(filename.c_str(), ios::out | ios::trunc);
  if(fb->is_open()) {
    os_->rdbuf(fb);
    file_ = filename;
    max_size_ = max_size;
    generation_ = 1;
  } else {
    delete fb;
#ifndef _NDEBUG
    throw std::runtime_error("open log file failed");
#endif
  }
}

void logger_impl::async_log(std::string const &data)
{
  if(!os_.get()) return;
  
  rotate();
  (*os_) << data ;
  os_->flush();
}

void logger_impl::rotate()
{
  using namespace std;
  if( file_.size() && os_->tellp() > max_size_ ) {
    string rotate_file;  
    filebuf *fb = 0;
    
    rotate_file = file_ + "." + boost::lexical_cast<string>(generation_);
    generation_++;
    os_->flush();
    delete os_->rdbuf();
    rename(file_.c_str(), rotate_file.c_str());
    fb = new filebuf();
    fb->open(file_.c_str(), ios::out | ios::trunc);
    if(fb->is_open())
      os_->rdbuf(fb);
    else {
      delete fb;
#ifndef _NDEBUG
      throw std::runtime_error("open log file failed");
#endif 
    }
    if ( generation_ > 32 ) archive();
  }
}

void logger_impl::archive()
{
  using namespace std;
  using boost::shared_ptr;

  // initialize libarchive handle
  shared_ptr<struct archive> ar(archive_write_new(), &archive_write_free);
  if( !ar ||  
      ARCHIVE_FATAL == archive_write_set_format_pax_restricted(ar.get()) ||
      ARCHIVE_FATAL == archive_write_add_filter_gzip(ar.get())  ||
      ARCHIVE_FATAL == archive_write_open_filename(ar.get(), (file_ + ".tgz").c_str())
    )
  {
    return;
  }
  // initaialize archive entry
  shared_ptr<archive_entry> ar_entry(archive_entry_new(), &archive_entry_free);
  if(!ar_entry) return;
  char buf[8192];
  for(int suffix = 1; suffix < generation_ ; ++suffix) {
    string filename = file_ + "." + boost::lexical_cast<string>(suffix);
    struct stat sb;
    stat(filename.c_str(), &sb);
    archive_entry_copy_stat(ar_entry.get(), &sb);
    archive_entry_set_pathname(ar_entry.get(), filename.c_str());
    archive_entry_set_filetype(ar_entry.get(), AE_IFREG);
    archive_write_header(ar.get(), ar_entry.get());
    ifstream fin(filename.c_str(), ios::in | ios::binary);
    while(fin.read(buf, sizeof(buf))) 
      archive_write_data(ar.get(), buf, (size_t)fin.gcount());
    fin.close();
    remove(filename.c_str());
    archive_entry_clear(ar_entry.get());
  }
  archive_write_close(ar.get());
  generation_ = 1;
}

void logger_impl::run()
{
  // XXX It's tricky to setup a lock here. Though, this is the
  // only way I know to keep log finish its tasks.
  boost::unique_lock<boost::mutex> lock(mutex_);
  io_service_->run();
}
