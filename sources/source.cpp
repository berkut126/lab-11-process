// Copyright 2018 Your Name <your_email>
#include <header.hpp>
int run(std::string& buildOption, variables_map& vm){
    boost::filesystem::path pathToCmake = boost::process::search_path("cmake");
    boost::process::group cmakeGroup;
    auto b = async::spawn([&]{
        boost::process::child cmake(
                pathToCmake,
                "-H. -B_builds -DCMAKE_INSTALL_PREFIX"
                "=_install -DCMAKE_BUILD_TYPE="
                +buildOption,
                boost::process::std_out > stdout,
                cmakeGroup);
        cmake.wait();
        return cmake.exit_code();
    });
    b.then([&](int a) -> int{
        if (a != 0) {
            return 1;
        } else {
            boost::process::child cmake(
                    pathToCmake, "--build _builds",
                    boost::process::std_out > stdout,
                    cmakeGroup);
            cmake.wait();
            return cmake.exit_code();
        }
    });
    b.then([&](int a){
        if (vm.count("install") && a){
            boost::process::child cmake(
                    pathToCmake,
                    "--build _builds --target install",
                    boost::process::std_out > stdout,
                    cmakeGroup);
            cmake.wait();
            return cmake.exit_code();
        } else {
            return 1;
        }
    });
    b.then([&](int a){
        if (vm.count("install") && a){
            boost::process::child cmake(
                    pathToCmake,
                    "--build _builds --target package",
                    boost::process::std_out > stdout,
                    cmakeGroup);
            cmake.wait();
            return cmake.exit_code();
        } else {
            return 1;
        }
    });
    b.wait();
    return b.get();
}

int main(int argc, char* argv[]){
    std::string buildOption("Debug");
    options_description desc{"Allowed options"};
    desc.add_options()
            ("help",
             "выводим вспомогательное сообщение")
            ("config",
             value<std::string>(),
             "указываем конфигурацию сборки")
            ("install",
             "добавляем этап установки (в директорию _install)")
            ("pack",
             "добавляем этап упаковки (в архив формата tar.gz)")
            ("timeout",
             value<int>(),
             "output file path");
    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);
    if (vm.count("help")) {
        std::cout << R"(Usage: builder [options]
Allowed options:
  --help                    : выводим вспомогательное сообщение
  --config <Release|Debug>  : указываем конфигурацию сборки (по умолчанию Debug)
  --install                 : добавляем этап установки (в директорию _install)
  --pack                    : добавляем этап упаковки (в архив формата tar.gz)
  --timeout <count>         : указываем время ожидания (в секундах))" << "\n";
        return 1;
    }
    if (vm.count("config")) {
        buildOption = vm["config"].as<std::string>();
    }
    if (vm.count("timeout")) {
        std::future a = std::async(
                std::launch::async,
                [&] { return run(buildOption, vm); });
        a.wait_for(
        std::chrono::seconds(vm["timeout"].as<int>()));
    } else {
        run(buildOption, vm);
    }
}
