#ifndef AGENT_PRE_COMPILE_OPTIONS_HPP_
#define AGENT_PRE_COMPILE_OPTIONS_HPP_
//
// Undefine or modify following macro to fit your needs
//

// Log request and response headers
#define AGENT_LOG_HEADERS 

// Log request and response body
#define AGENT_LOG_BODY 

// Maximum redirection times per request
#define AGENT_MAXIMUM_REDIRECT_COUNT 5

// Connection buffer inititate size
#define AGENT_CONNECTION_BUFFER_SIZE 2048

// Handler tracking
#define AGENT_ENABLE_HANDLER_TRACKING

#endif
