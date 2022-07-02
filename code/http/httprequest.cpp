#include "httprequest.h"

void
HttpRequest::init ()
{
    method_ = path_ = version_ = body_ = "";
    state_                             = REQUEST_LINE;
    header_.clear ();
    post_.clear ();
}