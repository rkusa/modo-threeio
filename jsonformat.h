#ifndef __threeio__json_format__
#define __threeio__json_format__

#include <iostream>
#include <fstream>
#include <stack>

#include <lxu_format.hpp>

class JSONFormat : public CLxFileFormat
{
public:
    JSONFormat();
    virtual ~JSONFormat();

    virtual bool ff_Open(const char *) override;
    virtual void ff_Enable(bool) override;
    virtual bool ff_HasError() override;
    virtual void ff_Cleanup() override;

    const char*	file_name;
    std::string filename_;

    const unsigned precision() const;
    void precision(unsigned);
    
    const bool pretty() const;
    void pretty(bool);

    void Write(std::nullptr_t);
    void Write(bool);
    void Write(int);
    void Write(unsigned);
    void Write(float);
    void Write(double);
    void Write(const char*);
    void Write(std::string);
    void Write(std::ifstream& is, std::string type);
    void WriteKey(std::string);
    void Property(std::string, bool);
    void WriteColor(const LXtVector& color);
    void Property(std::string, int);
    void Property(std::string, unsigned);
    void Property(std::string, float);
    void Property(std::string, double);
    void Property(std::string, std::string);
    void Property(std::string, const char*);
    void StartObject();
    void StartObject(std::string);
    void EndObject();
    void StartArray();
    void StartArray(std::string);
    void EndArray();
    
private:
    
    const std::string kBase64Enc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    // prevent implicit argument type conversion
    template<typename T>
    void Write(T);
    template<typename T>
    void Write(std::string, T);
    
    enum Context {
        kArray,
        kObject,
        kValue
    };

    const std::string kIndentWith = "\t";
    const std::string kLineBreakWith = "\n";
    
    std::ofstream* os_ = nullptr;
    std::stack<Context> context_;
    unsigned precision_ = 13;
    const char* precision_format_ = "%.13g";
    bool pretty_ = true;
    bool enabled_ = false;
    bool has_value_ = false;
    unsigned indention_ = 0;

    void BeforeWrite();
    void WriteIndention();
    void Newline();
};
#endif // /* defined(__threeio__threesaver__) */