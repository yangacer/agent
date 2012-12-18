
template<typename M>
void connection::read_some(M const &buffers, io_handler_type handler) // TODO timeout
{
  using namespace boost::asio;

  M &buffers_ = const_cast<M&>(buffers);

  if( iobuf_.size() ) {
    size_t copied = buffer_copy(buffers_, iobuf_.data());
    iobuf_.consume(copied);
    io_service_.post(
      boost::bind(handler, boost::system::error_code(), 
                  copied, shared_from_this()));
  } else {
    boost::asio::async_read(
      socket_,
      buffers,
      boost::asio::transfer_at_least(1),
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, 0, handler));
  }
}

template<typename M>
void connection::read(M const &buffers, io_handler_type handler) // TODO timeout
{
  using namespace boost::asio;

  M &buffers_ = const_cast<M&>(buffers);
  size_t offset = 0;
  size_t size = buffer_size(buffers_);
    
  if( iobuf_.size() ) {
    offset = buffer_copy(buffers_, iobuf_.data());
    iobuf_.consume(offset);
  }
  if( size > offset ) {
    boost::asio::async_read(
      socket_,
      buffer(buffers_ + offset),
      boost::asio::transfer_at_least(size - offset),
      boost::bind(&connection::handle_read, shared_from_this(), 
                  _1, _2, offset, handler));
  } else {
    io_service_.post(
      boost::bind(handler, boost::system::error_code(), 
                  offset, shared_from_this()));
  }
}
