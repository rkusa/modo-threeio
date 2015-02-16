#include "jsonformat.h"

#include <assert.h>

JSONFormat::JSONFormat()
{
}

JSONFormat::~JSONFormat()
{
    ff_Cleanup();
}

bool JSONFormat::ff_Open(const char *filename)
{
    ff_Cleanup();
    
    file_name = filename;
    
    os_ = new std::ofstream;
    os_->open(file_name, std::ifstream::out);
    
    enabled_ = true;
    return os_->good();
}

void JSONFormat::ff_Enable(bool enable)
{
    enabled_ = enable;
}

bool JSONFormat::ff_HasError()
{
    return !os_ || (!os_->eof() && !os_->good());
}

void JSONFormat::ff_Cleanup()
{
    file_name = nullptr;
    if (os_) {
        delete os_;
        os_ = nullptr;
    }
}

const unsigned JSONFormat::precision() const
{
    return precision_;
}

void JSONFormat::precision(unsigned precision)
{
    if (precision > 13) {
        precision = 13;
    }
    
    precision_ = precision;
    
    char buf[5];
    snprintf(buf, sizeof buf, "%%.%df", precision);
    precision_format_ = strdup(buf);
}

void JSONFormat::BeforeWrite()
{
    if (context_.size() > 0 && context_.top() == kValue) {
        context_.pop();
    } else if (context_.size() > 0 && context_.top() == kArray && has_value_) {
        *os_ << ", ";
    }
    
    has_value_ = true;
}

void JSONFormat::WriteIndention()
{
    for (unsigned i = 0; i < indention_; ++i) {
        *os_ << kIndentWith;
    }
}

void JSONFormat::Newline()
{
    *os_ << kLineBreakWith;
}

void JSONFormat::Write(std::nullptr_t)
{
    BeforeWrite();
    
    *os_ << "null";
}

void JSONFormat::Write(bool val)
{
    BeforeWrite();
    
    *os_ << (val ? "true" : "false");
}

void JSONFormat::Write(int val)
{
    BeforeWrite();
    
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d", val);
    *os_ << buf;
}

void JSONFormat::Write(unsigned val)
{
    BeforeWrite();
    
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d", val);
    *os_ << buf;
}

// http://stackoverflow.com/questions/2225956/what-is-the-sprintf-pattern-to-output-floats-without-ending-zeros
void morphNumericString(char *s) {
    char *p = strchr(s,'.');
    if (p == NULL) {
        strcat (s, ".0");
        return;
    }
    
    p = &(p[strlen(p)-1]);
    while ((p != s) && (*p == '0') && (*(p-1) != '.')) {
        *p-- = '\0';
    }
}

void JSONFormat::Write(double val)
{
    BeforeWrite();
    
    char buf[32];
    snprintf(buf, sizeof buf, precision_format_, val);
    
    // remove trailing zeros
    morphNumericString(buf);
    
    *os_ << buf;
}

void JSONFormat::Write(const char* val)
{
    BeforeWrite();
    
    *os_ << '"';
    
    // thanks to https://github.com/dropbox/json11
    auto len = strlen(val);
    for (unsigned i = 0; i < len; i++) {
        const char ch = val[i];
        if (ch == '\\') {
            *os_ << "\\\\";
        } else if (ch == '"') {
            *os_ << "\\\"";
        } else if (ch == '\b') {
            *os_ << "\\b";
        } else if (ch == '\f') {
            *os_ << "\\f";
        } else if (ch == '\n') {
            *os_ << "\\n";
        } else if (ch == '\r') {
            *os_ << "\\r";
        } else if (ch == '\t') {
            *os_ << "\\t";
        } else if (static_cast<uint8_t>(ch) <= 0x1f) {
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", ch);
            *os_ << buf;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(val[i+1]) == 0x80
                   && static_cast<uint8_t>(val[i+2]) == 0xa8) {
            *os_ << "\\u2028";
            i += 2;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(val[i+1]) == 0x80
                   && static_cast<uint8_t>(val[i+2]) == 0xa9) {
            *os_ << "\\u2029";
            i += 2;
        } else {
            *os_ << ch;
        }
    }
    
    *os_ << '"';
}

void JSONFormat::Write(std::string val)
{
    Write(val.c_str());
}

void JSONFormat::WriteKey(std::string str)
{
    assert(context_.size() > 0 && context_.top() == kObject);
    
    if (has_value_) {
        *os_ << ",";
    }
    Newline();
    WriteIndention();
    Write(str);
    *os_ << ": ";
    
    context_.push(kValue);
}

void JSONFormat::Property(std::string key, bool val)
{
    WriteKey(key);
    Write(val);
}

void JSONFormat::Property(std::string key, int val)
{
    WriteKey(key);
    Write(val);
}

void JSONFormat::Property(std::string key, unsigned val)
{
    WriteKey(key);
    Write(val);
}

void JSONFormat::Property(std::string key, double val)
{
    WriteKey(key);
    Write(val);
}

void JSONFormat::Property(std::string key, std::string val)
{
    WriteKey(key);
    Write(val);
}

void JSONFormat::Property(std::string key, const char* val)
{
    WriteKey(key);
    Write(val);
}

void JSONFormat::StartObject()
{
    assert(context_.size() == 0 || context_.top() == kValue || context_.top() == kArray);
    
    BeforeWrite();
    
    context_.push(kObject);
    has_value_ = false;
    
    indention_++;
    *os_ << '{';
}

void JSONFormat::StartObject(std::string key)
{
    WriteKey(key);
    StartObject();
}

void JSONFormat::EndObject()
{
    assert(context_.size() > 0 && context_.top() == kObject);
    context_.pop();
    
    if (has_value_) {
        Newline();
    }
    has_value_ = true;
    
    indention_--;
    WriteIndention();
    *os_ << '}';
}

void JSONFormat::StartArray()
{
    assert(context_.size() == 0 || context_.top() == kValue);
    
    BeforeWrite();
    
    context_.push(kArray);
    has_value_ = false;
    
    *os_ << '[';
}

void JSONFormat::StartArray(std::string key)
{
    WriteKey(key);
    StartArray();
}

void JSONFormat::EndArray()
{
    assert(context_.size() > 0 && context_.top() == kArray);
    context_.pop();
    has_value_ = true;
    
    *os_ << ']';
}
