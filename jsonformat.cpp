#include "jsonformat.h"

#include <assert.h>

#define ENABLED if (!enabled_) return;

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
    
    
    filename_ = filename;
    file_name = filename_.c_str();
    
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
    filename_ = "";
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

const bool JSONFormat::pretty() const
{
    return pretty_;
}

void JSONFormat::pretty(bool pretty)
{
    pretty_ = pretty;
}

void JSONFormat::BeforeWrite()
{
    if (context_.size() > 0 && context_.top() == kValue) {
        context_.pop();
    } else if (context_.size() > 0 && context_.top() == kArray && has_value_) {
        *os_ << (pretty_ ? ", " : ",");
    }
    
    has_value_ = true;
}

void JSONFormat::WriteIndention()
{
    if (!pretty_) {
        return;
    }
    
    for (unsigned i = 0; i < indention_; ++i) {
        *os_ << kIndentWith;
    }
}

void JSONFormat::Newline()
{
    if (!pretty_) {
        return;
    }
    
    *os_ << kLineBreakWith;
}

void JSONFormat::Write(std::nullptr_t)
{
    ENABLED
    
    BeforeWrite();
    
    *os_ << "null";
}

void JSONFormat::Write(bool val)
{
    ENABLED
    
    BeforeWrite();
    
    *os_ << (val ? "true" : "false");
}

void JSONFormat::Write(int val)
{
    ENABLED
    
    BeforeWrite();
    
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d", val);
    *os_ << buf;
}

void JSONFormat::Write(unsigned val)
{
    ENABLED
    
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

void JSONFormat::Write(float val)
{
    Write((double)val);
}

void JSONFormat::Write(double val)
{
    ENABLED
    
    BeforeWrite();
    
    char buf[32];
    snprintf(buf, sizeof buf, precision_format_, val);
    
    // remove trailing zeros
    morphNumericString(buf);
    
    *os_ << buf;
}

void JSONFormat::Write(const char* val)
{
    ENABLED
    
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

inline void a3_to_a4(unsigned char * a4, unsigned char * a3) {
    a4[0] = (a3[0] & 0xfc) >> 2;
    a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
    a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
    a4[3] = (a3[2] & 0x3f);
}

void JSONFormat::Write(std::ifstream& is, std::string type)
{
    ENABLED
    
    BeforeWrite();
    
    *os_ << '"';
    *os_ << "data:" + type + ";base64,";
    
    // thanks to https://github.com/adamvr/arduino-base64/blob/master/Base64.cpp
    int i = 0, j = 0;
    unsigned char a3[3];
    unsigned char a4[4];
    
    while(!is.eof()) {
        a3[i++] = is.get();
        if(i == 3) {
            a3_to_a4(a4, a3);
            
            for(i = 0; i < 4; i++) {
                *os_ << kBase64Enc[a4[i]];
            }
            
            i = 0;
        }
    }
    
    if(i) {
        for(j = i; j < 3; j++) {
            a3[j] = '\0';
        }
        
        a3_to_a4(a4, a3);
        
        for(j = 0; j < i + 1; j++) {
            *os_ << kBase64Enc[a4[i]];
        }
        
        while((i++ < 3)) {
            *os_ << '=';
        }
    }
    
    *os_ << '"';
}

void JSONFormat::WriteKey(std::string str)
{
    ENABLED
    
    assert(context_.size() > 0 && context_.top() == kObject);
    
    if (has_value_) {
        *os_ << ",";
    }
    Newline();
    WriteIndention();
    Write(str);
    *os_ << (pretty_ ? ": " : ":");
    
    context_.push(kValue);
}

void JSONFormat::WriteColor(const LXtVector& color)
{
    ENABLED
    
    BeforeWrite();
    
    int val = ((int(color[0] * 255) & 0xff) << 16) +
              ((int(color[1] * 255) & 0xff) << 8) +
              ((int(color[2] * 255) & 0xff));
    
    *os_ << val;
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

void JSONFormat::Property(std::string key, float val)
{
    Property(key, (double)val);
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
    ENABLED
    
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
    ENABLED
    
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
    ENABLED
    
    assert(context_.size() == 0 || context_.top() == kValue || context_.top() == kArray);
    
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
    ENABLED
    
    assert(context_.size() > 0 && context_.top() == kArray);
    context_.pop();
    has_value_ = true;
    
    *os_ << ']';
}
