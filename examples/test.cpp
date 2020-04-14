#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>

#include <upcxx/upcxx.hpp>
#include "collectives.hpp"

int main() {
    upcxx::init();

    {
        const std::int64_t N = 16;
        std::vector<std::int64_t> input(N), output(N*upcxx::rank_n());

        if(upcxx::rank_me() > 0) {
            std::fill(input.begin(), input.end(), 100);
        }
        else {
            std::fill(input.begin(), input.end(), 0);
        }

        std::fill(output.begin(), output.end(), 0);

        upcxx::utils::collectives::nonblocking_gather nbg{};
        nbg(input.begin(), input.end(), output.begin()); 

        if(upcxx::rank_me() == 0) {
            for(const auto& val : output) { std::cout << val << std::endl; }
        }
    }

    {
        const std::int64_t N = 16;
        std::vector<std::int64_t> input(N);
        std::fill(input.begin(), input.end(), 100);
        std::int64_t output{0};

        upcxx::utils::collectives::nonblocking_reduce nbr{};
        nbr(std::execution::seq, input.begin(), input.end(), 0, std::plus<std::int64_t>(), output); 

        if(upcxx::rank_me() == 0) {
            std::cout << output << std::endl;
        }
    }

    upcxx::finalize();
}
