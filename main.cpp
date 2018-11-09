//
// Created by nicolas on 07/11/18.
//

# include <unordered_map>
# include <fstream>
# include <iostream>
# include <cstring>
# include <regex>
# include <functional>
# include <utility>
# include <iterator>
# include <type_traits>
# include <unordered_set>
# include <set>

class request
{
public:
    request(unsigned long timestamp, std::string request)
     : request_(std::move(request))
     , timestamp_(timestamp)
    { }

    request() = default;

    unsigned long get_timestamp() const { return timestamp_; }
    const std::string& get_request() const {return request_; }
    std::string& get_request() {return request_; }

private:
    std::string request_;
    unsigned long timestamp_;
};


class line : public std::string {};
std::istream &operator>>(std::istream &is, line &l)
{
    std::getline(is, l);
    return is;
}


static request producer(const line& line)
{
    std::stringstream sstr(line);
    unsigned long timestamp;
    std::string req;
    sstr >> timestamp;
    sstr >> req;
    return request(timestamp, req);
}


template<typename input_it_t>
class base_iterator
{
public:
    using base_iterator_t = base_iterator<input_it_t>;

    explicit base_iterator(input_it_t in)
     : in_(in)
    { }

    bool operator==(const base_iterator_t& other) const { return other.in_ == in_; }
    bool operator!=(const base_iterator_t& other) const { return other.in_ != in_; }

protected:
    input_it_t in_;
};


template<typename input_it_t, typename value_type_t>
class transform_input_iterator : public base_iterator<input_it_t>
{
public:
    using base_t = base_iterator<input_it_t>;
    using transform_input_iterator_t = transform_input_iterator<input_it_t, value_type_t>;
    using func_t = std::function<value_type_t(typename input_it_t::value_type)>;
    using value_type = value_type_t;

    transform_input_iterator(input_it_t in, func_t f)
     : base_t(in)
     , f_(std::move(f))
    { }


    value_type& operator *()
    {
        if (!defined_)
        {
            defined_ = true;
            cur_ = f_(*base_t::in_);
        }
        return cur_;
    }

    value_type* operator->()
    {
        **this;
        return &cur_;
    }

    transform_input_iterator_t& operator++()
    {
        defined_ = false;
        base_t::in_++;
        return *this;
    }

    transform_input_iterator_t operator++(int)
    {
        transform_input_iterator_t cpy(*this);
        ++(*this);
        return cpy;
    }

private:
    func_t f_;
    bool defined_ = false;
    value_type cur_;

};


template<typename input_it_t>
class filter_input_iterator : public base_iterator<input_it_t>
{
public:
    using base_t = base_iterator<input_it_t>;
    using filter_input_iterator_t = filter_input_iterator<input_it_t>;
    using value_type = typename input_it_t::value_type;
    using func_t = std::function<bool(typename input_it_t::value_type)>;

    filter_input_iterator(input_it_t in, input_it_t end, func_t f)
     : base_t(in)
     , end_(end)
     , f_(f)
    { }

    typename input_it_t::value_type& operator *() { return *base_t::in_; }
    input_it_t operator->() { return base_t::in_; }

    filter_input_iterator_t& operator++()
    {
        do
            base_t::in_++;
        while(base_t::in_ != end_ && !f_(*base_t::in_));
        return *this;
    }

    filter_input_iterator_t operator++(int)
    {
        filter_input_iterator_t cpy(*this);
        ++(*this);
        return cpy;
    }

private:
    input_it_t end_;
    func_t f_;
};


template<typename iterator_t>
void compute_distinct(iterator_t begin, iterator_t end)
{
    std::unordered_set<std::string> distinct;
    auto distinct_counter = [&distinct](request& r) { distinct.insert(std::move(r.get_request())); };
    std::for_each(begin, end, distinct_counter);
    std::cout << distinct.size() << std::endl;
}


template<typename iterator_t>
void compute_n_most_common(iterator_t begin, iterator_t end, unsigned top_n)
{
    std::unordered_multiset<std::string> freq_set;
    auto distinct_counter = [&freq_set](request& r) { freq_set.insert(std::move(r.get_request())); };
    std::for_each(begin, end, distinct_counter);
    std::set<std::pair<unsigned, std::string>> most_common;
    unsigned worse_n = 0;
    auto top_counter = [&freq_set, &most_common, &worse_n, top_n](const std::string& req) {
        size_t count = freq_set.count(req);
        if (count > worse_n)
        {
            most_common.emplace(count, req);
            if (most_common.size() > top_n)
                most_common.erase(*most_common.begin());
            worse_n = most_common.begin()->first;
        }
    };
    std::for_each(freq_set.begin(), freq_set.end(), top_counter);
    for (auto it = most_common.rbegin(); it != most_common.rend(); it++)
    {
        std::cout << it->second << " " << it->first << std::endl;
    }
}


auto build_pipeline(std::ifstream& file, size_t from, size_t to)
{
    std::istream_iterator<line> begin(file);
    std::istream_iterator<line> end;

    transform_input_iterator<decltype(begin), request> it1(begin, producer);
    transform_input_iterator<decltype(begin), request> it1_end(end, producer);

    auto sup_predicate = [from](request b) { return from < b.get_timestamp(); };
    filter_input_iterator<decltype(it1)> it2(it1, it1_end, sup_predicate);
    filter_input_iterator<decltype(it1)> it2_end(it1_end, it1_end, sup_predicate);

    auto inf_predicate = [to](request b) { return to > b.get_timestamp(); };
    filter_input_iterator<decltype(it2)> pipeline_begin(it2, it2_end, inf_predicate);
    filter_input_iterator<decltype(it2)> pipeline_end(it2_end, it2_end, inf_predicate);

    return std::make_pair(pipeline_begin, pipeline_end);
}


int main(int argc, char* argv[])
{
    size_t from = 0;
    size_t to = UINT64_MAX;
    std::string filename(argv[argc - 1]);
    unsigned top_n = 10;

    std::ifstream inputFile(filename);
    auto pipeline = build_pipeline(inputFile, from, to);

    //compute_distinct(pipeline.first, pipeline.second);
    compute_n_most_common(pipeline.first, pipeline.second, top_n);

    return 0;
}
