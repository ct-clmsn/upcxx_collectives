//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <iostream>
#include <vector>
#include <algorithm>
//#include <execution>

#include <upcxx/upcxx.hpp>
#include "upcxx_collectives/collectives.hpp"

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

    upcxx::barrier();

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

    upcxx::barrier();

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

    upcxx::barrier();

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

    upcxx::barrier();

std::cout << "tree binomial scatter" << std::endl;
    {
        const std::int64_t N = 16;
        std::vector<std::int64_t> input(N*upcxx::rank_n()), output(N);

        if(upcxx::rank_me() < 1) {
            for(std::int64_t i = 0; i < (N*upcxx::rank_n()); ++i) {
                input[i] = (i / N);
            }

            //std::fill(input.begin(), input.end(), 100);
        }
        else {
            std::fill(input.begin(), input.end(), 0);
        }

        std::fill(output.begin(), output.end(), 0);

        upcxx::utils::collectives::nonblocking_binomial_scatter nbg{};
        nbg(input.begin(), input.end(), output.begin()); 

        //if(upcxx::rank_me() > 0) {
            for(const auto& val : output) { std::cout << val << std::endl; }
        //}
    }

std::cout << "tree binary scatter" << std::endl;
    {
        const std::int64_t N = upcxx::rank_n();
        std::vector<std::int64_t> input(N * N), output(N);

        if(upcxx::rank_me() < 1) {
            std::fill(input.begin(), input.end(), 100);
        }
        else {
            std::fill(input.begin(), input.end(), 0);
        }

        std::fill(output.begin(), output.end(), 0);

        upcxx::utils::collectives::nonblocking_binary_scatter nbg{};
        nbg(input.begin(), input.end(), output.begin()); 

        if(upcxx::rank_me() > 0) {
            for(const auto& val : output) { std::cout << upcxx::rank_me() << ' ' << val << std::endl; }
        }
    }
    upcxx::barrier();

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

    upcxx::barrier();

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

    upcxx::barrier();
    upcxx::finalize();
}
