#include <SaneDev.hpp>

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <map>

void write_pnm_header( SANE_Parameters& parm, FILE* out )
{
  switch( parm.format )
  {
    case SANE_FRAME_RED:
    case SANE_FRAME_GREEN:
    case SANE_FRAME_BLUE:
    case SANE_FRAME_RGB:
      fprintf(out, "P6\n# SANE data follows\n%d %d\n%d\n", parm.pixels_per_line, parm.lines, (parm.depth <= 8) ? 255 : 65535);
      break;
    default:
      if (parm.depth == 1)
        fprintf(out, "P4\n# SANE data follows\n%d %d\n", parm.pixels_per_line, parm.lines);
      else
        fprintf(out, "P5\n# SANE data follows\n%d %d\n%d\n", parm.pixels_per_line, parm.lines, (parm.depth <= 8) ? 255 : 65535);
      break;
  }
}

int main( int argc, char* argv[] )
{
  int deviceIdx = 0;
  int verbose = 0;
  const char* outputfile = "output.pnm";

  sane_init(0,0);

  sanepp::SaneDev dev;

  std::vector<sanepp::SaneDevDescriptor> deviceList;
  sanepp::Status status = dev.GetDevices( deviceList );
  if (status != sanepp::Status::Good )
  {
    std::cerr<<"Failed to get device list"<<std::endl;
    return -1;
  }
  std::map<std::string,std::string> selectedOptions;
  int c;
  while( (c = getopt(argc, argv, "hvLd:f:o:") ) != -1 )
  {
    switch(c)
    {
    case 'h':
      std::cout<<"Usage: "<<argv[0]<<" [-h] [-v] [-L] [-f filename] [-d devicenumber] [-o optionname=optionvalue]"<<std::endl;  
      return 0;
    case 'L':
      std::cout<<"Devices:"<<std::endl;
      for (auto& device : deviceList)
      {
        std::cout<<"\t"<<device.name<<std::endl;
      }
      return 0;
    case 'd':
      sscanf(optarg, "%d", &deviceIdx);
      break;
    case 'f':
      outputfile = optarg;
      break;
    case 'v':
      verbose++;
      break;
    case 'o':
      char* optVal = strchr(optarg,'=');
      if (optVal)
      {
        optVal[0] = '\0';
        optVal++;
        selectedOptions.insert( std::make_pair<std::string,std::string>(optarg,optVal));
      }
      break;
    }
  }
  if (deviceIdx < 0)
  {
    std::cerr<<"Invalid device index: "<<deviceIdx<<std::endl;
    return -1;
  }
  if (deviceIdx >= deviceList.size() )
  {
    std::cerr<<"Device not found"<<std::endl;
    return -1;
  }
  if (verbose)
    std::cout<<"Open scanner: "<<deviceList[deviceIdx].name<<std::endl;
  status = dev.Open( deviceList[deviceIdx].name );
  if (status != sanepp::Status::Good)
  {
    std::cerr<<"Device "<<deviceList[deviceIdx].name<<" could not be accessed: "<<std::to_string(status)<<std::endl;
    return -1;
  }

  std::vector<sanepp::Option> options;
  status = dev.GetOptions( options );
  if (status != sanepp::Status::Good)
  {
    std::cerr<<"Failed to retrieve options"<<std::endl;
  }
  if (verbose)
    std::cout<<"Options: "<<options.size()<<std::endl;
  for (int i=0; i<options.size(); ++i )
  {
    auto option = options[i];
    if (verbose)
      std::cout<<"  "<<option.name<<": "<<option.title<<" Size: "<<option.size<<(option.readonly?" (Readonly) ":" ")<<std::endl;
    switch( option.type )
    {
    case sanepp::OptionValueType::Boolean:
      {
        bool boolval = 0;
        status = dev.GetOption( option.name, boolval);
        if (verbose)
          std::cout<<"    Bool Value: "<<boolval<<std::endl;
      }
      break;
    case sanepp::OptionValueType::Integer:
      {
        int intval = 0;
        status = dev.GetOption( option.name, intval);
        if (verbose)
        {
          std::cout<<"    Int Value: "<<intval<<std::endl;
          for ( auto intVal : option.constraintIntegerList )
          {
              std::cout<<"         "<<intVal<<std::endl;
          }
          if ( option.constraintIntegerRange.min != option.constraintIntegerRange.max)
          {
              std::cout<<"    Constraint range: "<<(option.constraintIntegerRange.min)<<" "<<(option.constraintIntegerRange.max)<<" "<<(option.constraintIntegerRange.quant)<<std::endl;
          }
        }
      }
      break;
    case sanepp::OptionValueType::String:
      {
        std::string strval;
        status = dev.GetOption( option.name, strval);
        if (verbose)
        {
          std::cout<<"    Str Value: "<<strval<<std::endl;
          for ( auto strVal : option.constraintStringList )
          {
            std::cout<<"         "<<strVal<<std::endl;
          }
        }
        if (selectedOptions.count(option.name) > 0)
        {
          status = dev.SetOption( option.name, selectedOptions[option.name] );
          status = dev.GetOption( option.name, strval);
          if (verbose)
            std::cout<<"    Str Value: "<<strval<<std::endl;
        }
      }
      break;
    case sanepp::OptionValueType::Float:
      {
        double dval = 0.0;
        status = dev.GetOption( option.name, dval);
        if (verbose)
        {
          std::cout<<"    Float Value: "<<dval<<std::endl;
          if ( option.constraintFloatRange.min != option.constraintFloatRange.max)
          {
            std::cout<<"    Constraint range: "<<(option.constraintFloatRange.min)<<" "<<(option.constraintFloatRange.max)<<" "<<(option.constraintFloatRange.quant)<<std::endl;
          }
        }
      }
      break;
    }
  }

  std::cout<<"Start scanning:"<<std::endl;
  status = dev.Start();
  if (status != sanepp::Status::Good)
  {
    std::cerr<<"Device "<<deviceList[deviceIdx].name<<" could not start scanning: "<<std::to_string(status)<<std::endl;
    return -1;
  }

  SANE_Parameters parm;
  status = dev.GetParameters( parm );
  if (status != sanepp::Status::Good)
  {
    std::cerr<<"Device "<<deviceList[deviceIdx].name<<" could not get parameters: "<<std::to_string(status)<<std::endl;
    return -1;
  }


  FILE* fout = fopen( outputfile, "wb");
  write_pnm_header( parm, fout );

  std::vector<unsigned char> data;
  while ( (status = dev.Read( data, 4096 )) != sanepp::Status::Eof)
  {
    if (status != sanepp::Status::Good)
    {
      std::cerr<<"Scanning interrupted."<<std::endl;
      fclose( fout );
      return -1;
    }
    std::cout<<"Read "<<data.size()<<" bytes."<<std::endl;
    fwrite( data.data(), 1, data.size(), fout );
    data.clear();
  }
  fclose( fout );
  return 0;
}
