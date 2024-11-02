#include "serializer.h"

namespace serialization {

void SaveApp(app::Application& app) {
    std::string file_name = app.GetStateFile();
    if ( file_name.empty() ) {
        return;
    }
    //
    std::string new_file = file_name + ".new";
    std::ofstream ofs{new_file};
    boost::archive::text_oarchive oa{ofs};
    AppRepr app_repr(app);
    oa << app_repr;
    std::filesystem::rename(new_file, file_name);
}

void RestoreApp(app::Application& app) {
    std::string file_name = app.GetStateFile();
    if ( file_name.empty() ) {
        return;
    }
    if ( !std::filesystem::exists(file_name) ) {
        return;
    }
    //
   std::ifstream ifs{file_name};
   boost::archive::text_iarchive ia{ifs};
   AppRepr app_repr;
   ia >> app_repr;
   app_repr.Restore(app);
}



}  // namespace serialization
