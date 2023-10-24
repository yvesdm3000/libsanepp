#pragma once

#include <string>
#include <vector>
#include <sane/sane.h>

namespace sanepp {

struct SaneDevDescriptor {
  std::string name;
  std::string vendor;
  std::string model;
  std::string type;
  SaneDevDescriptor( const std::string& _name,  const std::string& _vendor,  const std::string& _model,  const std::string& _type ) :
	  name(_name), vendor(_vendor), model(_model), type(_type) {}
};
enum class Status {
    Good = SANE_STATUS_GOOD,                  /* everything A-OK */
    Unsupported = SANE_STATUS_UNSUPPORTED,    /* operation is not supported */
    Cancelled = SANE_STATUS_CANCELLED,        /* operation was cancelled */
    DeviceBusy = SANE_STATUS_DEVICE_BUSY,     /* device is busy; try again later */
    Inval = SANE_STATUS_INVAL,                /* data is invalid (includes no dev at open) */
    Eof = SANE_STATUS_EOF,                    /* no more data available (end-of-file) */
    Jammed = SANE_STATUS_JAMMED,              /* document feeder jammed */
    NoDocs = SANE_STATUS_NO_DOCS,             /* document feeder out of documents */
    CoverOpen = SANE_STATUS_COVER_OPEN,       /* scanner cover is open */
    IOError = SANE_STATUS_IO_ERROR,           /* error during device I/O */
    NoMem = SANE_STATUS_NO_MEM,               /* out of memory */
    AccessDenied = SANE_STATUS_ACCESS_DENIED  /* access to resource has been denied */
};

enum class OptionValueType {
  Boolean = SANE_TYPE_BOOL,
  Integer = SANE_TYPE_INT,
  Float   = SANE_TYPE_FIXED,
  String  = SANE_TYPE_STRING,
  Button  = SANE_TYPE_BUTTON,
  Group   = SANE_TYPE_GROUP
};

struct IntegerRange {
  int min;
  int max;
  int quant;
  IntegerRange() : min(0), max(0), quant(0){}
};
struct FloatRange {
  double min;
  double max;
  double quant;
  FloatRange() : min(0), max(0), quant(0){}
};

struct Option {
  int id;
  std::string name;  /* Short name of the option (command line option) */
  std::string title; /* Longername of the option */
  bool readonly;     /* If the option can be modified */
  OptionValueType type;
  size_t size;

  std::vector<int> constraintIntegerList;
  std::vector<std::string> constraintStringList;
  IntegerRange constraintIntegerRange;
  FloatRange constraintFloatRange;

  Option( int _id, const std::string& _name, const std::string& _title, bool _readonly, OptionValueType _type, size_t _size ):
    id(_id), name(_name), title(_title), readonly(_readonly), type(_type), size(_size){}
};

class SaneDev {
  SANE_Handle _h;
public:
  SaneDev();
  ~SaneDev();

  /**
   * Detect devices and return the detected devices. Can be called before the 'Open' call
   * */
  Status GetDevices( std::vector<SaneDevDescriptor>& out_deviceList );

  /**
   * Open a specific device.
   * */
  Status Open( const std::string& device );

  /**
   * Close a specific device.
   * Valid only after an Open() call
   * */
  Status Close();

  /**
   * Valid only after an Open() call
   */
  Status GetOptions( std::vector<Option>& out_options ) const;
  /**
   * Valid only after an Open() call
   */
  Status GetOption( const std::string& name, bool& out_value ) const;
  Status GetOption( const std::string& name, double& out_value ) const;
  Status GetOption( const std::string& name, int& out_value ) const;
  Status GetOption( const std::string& name, std::string& out_value ) const;
  /**
   * Valid only after an Open() call
   */
  Status SetOption( const std::string& name, bool value );
  Status SetOption( const std::string& name, double value );
  Status SetOption( const std::string& name, int value );
  Status SetOption( const std::string& name, const std::string& value );

  /**
   * Valid only after an Open() call
   */
  Status Start();

  /**
   * Retrieves metadata from the currently scanned document.
   * Valid only after a Start() call
   */
  Status GetParameters( SANE_Parameters& out_parameters );

  /**
   * Reads data from the scanner and *appends* them to out_data
   * Valid only after a Start() call
   */
  Status Read( std::vector<unsigned char>& out_data, int dataLen );

  /**
   * Valid only after a Start() call
   */
  Status Cancel();
};

};

namespace std {
  std::string to_string( sanepp::Status& status );
};
