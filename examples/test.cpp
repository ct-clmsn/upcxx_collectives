#include <iostream>
#include <vector>
#include <algorithm>
//#include <execution>

#include <upcxx/upcxx.hpp>
#include "collectives.hpp"

int main() {
    upcxx::init();

std::cout << "tree binomial broadcast" << std::endl;
    {
        std::int64_t N = 0;

        if(upcxx::rank_me() < 1) {
            N = 16;
        }

        upcxx::utils::collectives::nonblocking_binomial_broadcast nbb{};
        nbb(N);

        std::cout << upcxx::rank_me() << ' ' << N << std::endl;
    }

std::cout << "tree binary broadcast" << std::endl;
    {
        std::int64_t N = 0;

        if(upcxx::rank_me() < 1) {
            N = 16;
        }

        upcxx::utils::collectives::nonblocking_binomial_broadcast nbb{};
        nbb(N);

        std::cout << upcxx::rank_me() << ' ' << N << std::endl;
    }


std::cout << "tree binomial gather" << std::endl;
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

        upcxx::utils::collectives::nonblocking_binomial_gather nbg{};
        nbg(input.begin(), input.end(), output.begin()); 

        if(upcxx::rank_me() == 0) {
            for(const auto& val : output) { std::cout << val << std::endl; }
        }
    }

std::cout << "tree binary gather" << std::endl;
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

        upcxx::utils::collectives::nonblocking_binary_gather nbg{};
        nbg(input.begin(), input.end(), output.begin()); 

        if(upcxx::rank_me() == 0) {
            for(const auto& val : output) { std::cout << val << std::endl; }
        }
    }

std::cout << "tree binomial reduce" << std::endl;
    {
        const std::int64_t N = 16;
        std::vector<std::int64_t> input(N);
        std::fill(input.begin(), input.end(), 100);
        std::int64_t output{0};

        upcxx::utils::collectives::nonblocking_binomial_reduce nbr{};
        nbr(input.begin(), input.end(), 0, std::plus<std::int64_t>(), output); 

        if(upcxx::rank_me() == 0) {
            std::cout << output << std::endl;
        }
    }

std::cout << "tree binary reduce" << std::endl;
    {
        const std::int64_t N = 16;
        std::vector<std::int64_t> input(N);
        std::fill(input.begin(), input.end(), 100);
        std::int64_t output{0};

        upcxx::utils::collectives::nonblocking_binary_reduce nbr{};
        nbr(input.begin(), input.end(), 0, std::plus<std::int64_t>(), output); 

        if(upcxx::rank_me() == 0) {
            std::cout << output << std::endl;
        }
    }

    upcxx::finalize();
}
