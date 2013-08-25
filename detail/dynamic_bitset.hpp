#ifndef AGENT_DYNAMIC_BITSET_HPP_
#define AGENT_DYNAMIC_BITSET_HPP_

#define private protected
#include <boost/dynamic_bitset.hpp>
#undef private
#include <string>

template <
  typename Block = unsigned long, 
  typename Alloc = std::allocator<Block> >
class dynamic_bitset : public boost::dynamic_bitset<Block, Alloc>
{
  typedef boost::dynamic_bitset<Block, Alloc> super_;
  using super_::num_blocks;
  using super_::m_bits;
  using super_::bits_per_block;
  using super_::block_index;
  using super_::bit_index;
public:
  typedef typename super_::size_type size_type;
  typedef typename super_::block_width_type block_width_type;
  
  explicit
  dynamic_bitset(const Alloc& alloc = Alloc())
  : super_(alloc)
  {}

  explicit 
  dynamic_bitset(size_type num_bits, unsigned long value = 0,
                 const Alloc& alloc = Alloc())
  : super_(num_bits, value, alloc)
  {}

  template <typename BlockInputIterator>
  dynamic_bitset(BlockInputIterator first, BlockInputIterator last,
                 const Alloc& alloc = Alloc())
  : super_(first, last, alloc)
  {}

  template <typename CharT, typename Traits, typename CharAlloc>
  explicit dynamic_bitset(
    const std::basic_string<CharT, Traits, CharAlloc>& s,
    typename std::basic_string<CharT, Traits, CharAlloc>::size_type pos = 0)
  : super_(s, pos)
  {}


  dynamic_bitset(const dynamic_bitset& b)
  : super_(b)
  {}

  size_type find_first(bool on_off = true) const
  {
    return on_off ? super_::find_first() : m_do_find_off_from(0);
  }

  size_type find_next(size_type pos, bool on_off = true) const
  {
    if(on_off) return super_::find_next(pos);
    
    const size_type sz = super_::size();
    if( pos >= (sz-1) || 0 == sz)
      return super_::npos;
    
    ++pos;
    size_type const blk = block_index(pos);
    block_width_type ind = bit_index(pos);
    Block fore = ~m_bits[blk] & (~Block(0) << ind);

    size_type rt = fore ?
      blk * bits_per_block + boost::lowest_bit(fore) :
      m_do_find_off_from(blk + 1);
    return rt >= super_::size() ? super_::npos : rt;
  }
private:
  size_type m_do_find_off_from(size_type first_block) const
  {
    size_type i = first_block;

    while( i < num_blocks() && m_bits[i] == ~Block(0))
      ++i;
    if(i >= num_blocks())
      return super_::npos;
    size_type rt = i * bits_per_block + boost::lowest_bit(~m_bits[i]);
    return rt >= super_::size() ? super_::npos : rt;
  }
};

#endif
