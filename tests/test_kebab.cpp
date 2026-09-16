#include <print>
#include <starparse/starparse.hpp>

struct Args {
    int dry_run;
};

int main(const int argc, char **argv) {
    const auto args{StarParse::parse<Args>(argc, argv)};
    if (args) {
        std::println("dry run: {}", args->dry_run);
    } else {
        std::println("{}", args.error_message());
    }
}
