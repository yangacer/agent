#ifndef GAISWT_STATUS_CODE_HPP_
#define GAISWT_STATUS_CODE_HPP_

namespace http {
enum status_type
{
  ok = 200,
  created = 201,
  accepted = 202,
  no_content = 204,
  partial_content = 206,
  multiple_choices = 300,
  moved_permanently = 301,
  found = 302,
  see_other = 303,
  not_modified = 304,
  bad_request = 400,
  forbidden = 403,
  not_found = 404,
  out_of_range = 416,
  internal_server_error = 500,
  not_implemented = 501,
  bad_gateway = 502,
  service_unavailable = 503
};
} // namespace http

#endif // header
