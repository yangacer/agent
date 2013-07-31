#ifndef AGENT_PLACEHOLDERS_HPP_
#define AGENT_PLACEHOLDERS_HPP_


namespace agent_arg {

#if defined(__BORLANDC__) || defined(__GNUC__) && (__GNUC__ < 4)
// agent
static inline boost::arg<1> error   () { return boost::arg<1>(); }
static inline boost::arg<2> request () { return boost::arg<2>(); }
static inline boost::arg<3> response() { return boost::arg<3>(); }
static inline boost::arg<4> buffer  () { return boost::arg<4>(); }
static inline boost::arg<5> qos     () { return boost::arg<5>(); }
// monitor
static inline boost::arg<1> orient    () { return boost::arg<1>(); }
static inline boost::arg<2> total     () { return boost::arg<2>(); }
static inline boost::arg<3> transfered() { return boost::arg<3>(); }

#elif defined(BOOST_MSVC) || (defined(__DECCXX_VER) && __DECCXX_VER <= 60590031) || defined(__MWERKS__) || \
    defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 2)  
// agent
static boost::arg<1> error;
static boost::arg<2> request;
static boost::arg<3> response;
static boost::arg<4> buffer;
static boost::arg<5> qos;
// monitor
static boost::arg<1> orient;
static boost::arg<2> total;
static boost::arg<3> transfered;

#else
// agent
boost::arg<1> error;
boost::arg<2> request;
boost::arg<3> response;
boost::arg<4> buffer;
boost::arg<5> qos;
// monitor
boost::arg<1> orient;
boost::arg<2> total;
boost::arg<3> transfered;

#endif

} // namespace agent_arg

#endif
