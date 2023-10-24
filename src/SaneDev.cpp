#include "SaneDev.hpp"

using namespace sanepp;

namespace std {
  std::string to_string( sanepp::Status& status ) 
  {
    auto str = sane_strstatus( static_cast<SANE_Status>(status) );
    return str;
  }
};

SaneDev::SaneDev()
	: _h(nullptr)
{
  sane_init(0,0);
}
SaneDev::~SaneDev()
{
  if (_h)
    Close();
  sane_exit();
}

Status
SaneDev::GetDevices( std::vector<SaneDevDescriptor>& out_deviceList )
{
  const SANE_Device ** device_list;
  SANE_Status status = sane_get_devices( &device_list, true );
  if (status != SANE_STATUS_GOOD)
    return static_cast<Status>(status);
  out_deviceList.clear();
  for (int i=0; device_list[i]; ++i )
  {
    out_deviceList.push_back( SaneDevDescriptor( device_list[i]->name, device_list[i]->vendor, device_list[i]->model, device_list[i]->type ) );
  }
  return Status::Good;
}

Status
SaneDev::Open( const std::string& device )
{
  if (_h)
    Close();
  SANE_Status status = sane_open( device.c_str(), &_h );
  return static_cast<Status>(status);
}

Status
SaneDev::Close()
{
  sane_close(_h);

  return Status::Good;
}

Status
SaneDev::GetParameters( SANE_Parameters& out_parameters )
{
  SANE_Status status = sane_get_parameters( _h, &out_parameters );

  return static_cast<Status>(status);
}

Status
SaneDev::GetOptions( std::vector<Option>& out_options ) const
{
  SANE_Int num = 0;
  SANE_Status status = sane_control_option(_h, 0, SANE_ACTION_GET_VALUE, &num, 0);
  if (status != SANE_STATUS_GOOD)
    return static_cast<Status>(status);

  out_options.clear();

  const SANE_Option_Descriptor* opt;
  for (int i=1; i<num;++i)  // Option 0 is the amount of options
  {
    opt = sane_get_option_descriptor(_h, i);
    Option option(i, opt->name, opt->title, !SANE_OPTION_IS_SETTABLE(opt->cap), static_cast<sanepp::OptionValueType>( opt->type ), opt->size);
    switch(opt->constraint_type)
    {
    case SANE_CONSTRAINT_RANGE:
      if (opt->type == SANE_TYPE_FIXED)
      {
        option.constraintFloatRange.min = SANE_UNFIX(opt->constraint.range->min);
        option.constraintFloatRange.max = SANE_UNFIX(opt->constraint.range->max);
        option.constraintFloatRange.quant = SANE_UNFIX(opt->constraint.range->quant);
      }
      else
      {
        option.constraintIntegerRange.min = opt->constraint.range->min;
        option.constraintIntegerRange.max = opt->constraint.range->max;
        option.constraintIntegerRange.quant = opt->constraint.range->quant;
      }
      break;
    case SANE_CONSTRAINT_WORD_LIST:
      for (int j=1; j < opt->constraint.word_list[0]; ++j)
      {
        option.constraintIntegerList.push_back( opt->constraint.word_list[j] );
      }
      break;
    case SANE_CONSTRAINT_STRING_LIST:
      for (int j=0; opt->constraint.string_list[j]; ++j)
      {
        option.constraintStringList.push_back( opt->constraint.string_list[j] );
      }
      break;
    }
    out_options.push_back(std::move(option));
  }
  return Status::Good;
}

Status
SaneDev::GetOption( const std::string& name, int& out_value ) const
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::Integer)
         return Status::Inval;
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_GET_VALUE, &out_value, 0));
      return status;
    }
  }
  return Status::Unsupported;
}

Status
SaneDev::GetOption( const std::string& name, bool& out_value ) const
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::Boolean)
         return Status::Inval;
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_GET_VALUE, &out_value, 0));
      return status;
    }
  }
  return Status::Unsupported;
}

Status
SaneDev::GetOption( const std::string& name, double& out_value ) const
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::Float)
         return Status::Inval;
      int val;
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_GET_VALUE, &val, 0));
      out_value = SANE_UNFIX(val);
      return status;
    }
  }
  return Status::Unsupported;
}

Status
SaneDev::SetOption( const std::string& name, bool value )
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::Boolean)
         return Status::Inval;
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_SET_VALUE, &value, 0));
      return status;
    }
  }
  return Status::Unsupported;
}


Status
SaneDev::GetOption( const std::string& name, std::string& out_value ) const
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::String)
         return Status::Inval;
      char buffer[option.size+2];
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_GET_VALUE, buffer, 0));
      buffer[option.size-1] = '\0';
      out_value = buffer;
      return status;
    }
  }
  return Status::Unsupported;
}

Status
SaneDev::SetOption( const std::string& name, int value )
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::Integer)
         return Status::Inval;
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_SET_VALUE, &value, 0));
      return status;
    }
  }
  return Status::Unsupported;
}

Status
SaneDev::SetOption( const std::string& name, double value )
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::Float)
         return Status::Inval;
      int val = SANE_FIX(value);
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_SET_VALUE, &val, 0));
      return status;
    }
  }
  return Status::Unsupported;
}


Status
SaneDev::SetOption( const std::string& name, const std::string& value )
{
  std::vector<Option> options;
  Status status = GetOptions( options );
  if (status != Status::Good)
    return status;

  for (auto& option : options)
  {
    if (option.name == name)
    {
      if (option.type != OptionValueType::String)
         return Status::Inval;
      status = static_cast<Status>(sane_control_option(_h, option.id, SANE_ACTION_SET_VALUE, (void*)value.data(), 0));
      return status;
    }
  }
  return Status::Unsupported;
}



Status
SaneDev::Start()
{
  if (!_h)
    return Status::Inval;
  return static_cast<Status>(sane_start(_h));
}

Status
SaneDev::Read( std::vector<unsigned char>& out_data, int dataLen )
{
  int amount = 0;
  size_t offset = out_data.size();
  out_data.resize(offset + dataLen);
  Status ret = static_cast<Status>(sane_read( _h, out_data.data() + offset, dataLen, &amount ));
  if (ret == sanepp::Status::Good )
    out_data.resize(offset + amount);
  else
    out_data.resize(offset);
  return ret;
}

Status
SaneDev::Cancel()
{
  sane_cancel(_h);
  return Status::Good;
}
