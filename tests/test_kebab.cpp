#include <print>
#include <starparse/starparse.hpp>

struct Args {
    int dry_run;
};

int main(const int argc, char **argv) {
    const auto args{StarParse::try_parse<Args>(argc, argv)};
    if (args.has_value()) {
        std::println("dry run: {}", args->dry_run);
    } else {
        std::println("errors: {}", args.error());
    }
}
