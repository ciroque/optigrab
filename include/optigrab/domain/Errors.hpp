#pragma once

#include <stdexcept>
#include <string>

namespace optigrab {

class OptigrabError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DriveError : public OptigrabError {
public:
    using OptigrabError::OptigrabError;
};

class TocError : public OptigrabError {
public:
    using OptigrabError::OptigrabError;
};

class ExtractError : public OptigrabError {
public:
    using OptigrabError::OptigrabError;
};

class EncodeError : public OptigrabError {
public:
    using OptigrabError::OptigrabError;
};

class SessionError : public OptigrabError {
public:
    using OptigrabError::OptigrabError;
};

class ParseError : public OptigrabError {
public:
    using OptigrabError::OptigrabError;
};

}  // namespace optigrab
