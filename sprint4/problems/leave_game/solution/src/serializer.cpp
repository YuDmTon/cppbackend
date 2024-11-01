#include "serializer.h"

namespace serialization {

void SaveApp(app::Application& app) {
    std::string file_name = app.GetStateFile();
    if ( file_name.empty() ) {
        ////std::cout << "Don't save state" << std::endl;
        return;
    }
    ////std::cout << "Saving state..." << std::endl;
    //
    std::string new_file = file_name + ".new";
    std::ofstream ofs{new_file};
    boost::archive::text_oarchive oa{ofs};
    AppRepr app_repr(app);
    oa << app_repr;
    std::filesystem::rename(new_file, file_name);
    //
    ////std::cout << "Saving done." << std::endl;
}

void RestoreApp(app::Application& app) {
    std::string file_name = app.GetStateFile();
    if ( file_name.empty() ) {
        ////std::cout << "Don't restore state" << std::endl;
        return;
    }
    ////std::cout << "Try restore state" << std::endl;
    if ( !std::filesystem::exists(file_name) ) {
        ////std::cout << "File '" << file_name << "' doesn't exists. Cann't restore state" << std::endl;
        return;
    }
    ////std::cout << "Restoring state..." << std::endl;
    //
   std::ifstream ifs{file_name};
   boost::archive::text_iarchive ia{ifs};
   AppRepr app_repr;
   ia >> app_repr;
   app_repr.Restore(app);
   //
   ////std::cout << "Restoring done." << std::endl;
}



}  // namespace serialization
