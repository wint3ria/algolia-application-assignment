//
// Created by nicolas on 07/11/18.
//


#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <unordered_set>
#include <utility>

/*
 * Basic class representing a request, with its timestamp and text.
 */
class request
{
public: // Constructors
    request(unsigned long timestamp, std::string request)
     : request_(std::move(request))
     , timestamp_(timestamp)
    { }

    request() = default;

public: // Getters
    unsigned long get_timestamp() const { return timestamp_; }
    std::string& get_request() {return request_; }

private: // Attributes
    std::string request_;
    unsigned long timestamp_;
};

/*
 * Dummy line class to help getting each line in an input iterator
 */
class line : public std::string {};
// Overload the istream >> operator for the line class in order to use getline
std::istream &operator>>(std::istream &is, line &l)
{
    std::getline(is, l);
    return is;
}


/*
 * Simple line to request conversion function
 */
static request line_to_request(const line &line)
{
    std::stringstream sstr(line);
    unsigned long timestamp;
    std::string req;
    sstr >> timestamp;
    sstr >> req;
    return request(timestamp, req);
}


/*
 * Base class for our iterators.
 * Two classes are derived from it : transform and filter iterators.
 * Both of the subclass make use of a function of template type func_t, so at this point it is only saved in an
 * attribute.
 * input_it_t is the type of the input iterator on which this one streams its operations
 * value_type_t is the type of the values pointed by this iterator (return type of operators * and ->)
 */
template<typename input_it_t, typename value_type_t, typename func_t>
class base_iterator
{
public: // Constructors/Destructors
    using base_iterator_t = base_iterator<input_it_t, value_type_t, func_t>;
    using value_type = value_type_t;

    explicit base_iterator(input_it_t in, func_t f)
     : in_(std::move(in))
     , f_(std::move(f))
    { }

    // Make the class virtual in order to use dynamic_cast
    virtual ~base_iterator() = default;

public: // Operators
    // Comparison between two iterators
    inline bool operator==(const base_iterator_t& other) const { return other.in_ == in_; }
    inline bool operator!=(const base_iterator_t& other) const { return other.in_ != in_; }

    // Dereferencing operators. The derived class only have to update cur_ during the ++ operator.
    inline value_type& operator *() { return cur_; }
    inline value_type* operator->() { return &cur_; }

protected:
    // Method for factorisation of the postfix ++ operator accessible to derived classes.
    template<typename derived_t>
    inline derived_t increment_operator_postfix()
    {
        // We need dynamic_cast for downcasting to derived_t
        derived_t cpy = *dynamic_cast<derived_t*>(this);
        ++(*dynamic_cast<derived_t*>(this));
        return cpy;
    }

protected: // Attributes
    input_it_t in_;
    func_t f_;
    value_type cur_;
};


/*
 * Transform input iterator
 * Compliant with STL like input iterators
 * Transform each values in the iterator in_ by applying f_ on it.
 */
template<typename input_it_t, typename value_type_t>
class transform_input_iterator
 : public base_iterator<input_it_t, value_type_t, std::function<value_type_t(typename input_it_t::value_type)>>
{
public: // Constructors
    using transform_input_iterator_t = transform_input_iterator<input_it_t, value_type_t>;
    using func_t = std::function<value_type_t(typename input_it_t::value_type)>;
    using base_t = base_iterator<input_it_t, value_type_t, func_t>;
    using value_type = value_type_t;

    transform_input_iterator(input_it_t in, func_t f)
     : base_t(in, f)
    { }

public: // Operators
    inline transform_input_iterator_t& operator++()
    {
        base_t::in_++;
        base_t::cur_ = base_t::f_(*base_t::in_); // save the return value of f_ in cur_ for future dereferencing
        return *this;
    }

    inline transform_input_iterator_t operator++(int)
    {
        return base_t:: template increment_operator_postfix<transform_input_iterator_t>();
    }

};


/*
 * Filter input iterator
 * Compliant with STL like input iterators
 * Filter values of in_ by the predicate f_
 */
template<typename input_it_t>
class filter_input_iterator
 : public base_iterator<
         input_it_t,
         typename input_it_t::value_type,
         std::function<bool(typename input_it_t::value_type)> >
{
public: // Constructors
    using filter_input_iterator_t = filter_input_iterator<input_it_t>;
    using value_type = typename input_it_t::value_type;
    using func_t = std::function<bool(typename input_it_t::value_type)>;
    using base_t = base_iterator<input_it_t, value_type, func_t>;

    // This iterator needs to know the end of the input iterator in_
    filter_input_iterator(input_it_t in, input_it_t end, func_t f)
     : base_t(in, f)
     , end_(std::move(end))
    {
        // Search the first value matching the predicate f_
        if (base_t::in_ != end_ && !base_t::f_(*base_t::in_))
            ++(*this);
    }

public: // Operators
    inline filter_input_iterator_t& operator++()
    {
        // Find the next entry satisfying the predicate, if available
        do
            base_t::in_++;
        while(base_t::in_ != end_ && !base_t::f_(*base_t::in_));
        base_t::cur_ = *base_t::in_;
        return *this;
    }

    inline filter_input_iterator_t operator++(int)
    {
        return base_t:: template increment_operator_postfix<filter_input_iterator_t>();
    }

private: // Attributes
    input_it_t end_;
};


/*
 * Convenient "pipeline" building functions.
 * Each added iterator is considered as a layer.
 * Returns the new layer beginning and end iterators as a pair
 */

// Transform layer
template<typename input_it_t, typename transform_fun_t>
static inline auto add_transform_layer(input_it_t begin, input_it_t end, transform_fun_t f)
{
    transform_input_iterator<input_it_t, request> pipeline_begin(begin, f);
    transform_input_iterator<input_it_t, request> pipeline_end(end, f);
    return std::make_pair(pipeline_begin, pipeline_end);
}

// Filter layer
template<typename input_it_t, typename predicate_t>
static inline auto add_filter_layer(input_it_t begin, input_it_t end, predicate_t f)
{
    filter_input_iterator<input_it_t> pipeline_begin(begin, end, f);
    filter_input_iterator<input_it_t> pipeline_end(end, end, f);
    return std::make_pair(pipeline_begin, pipeline_end);
}


/*
 * Main processing pipeline.
 */
auto define_pipeline(std::ifstream &file, size_t from, size_t to)
{
    // layer: line
    std::istream_iterator<line> begin(file), end;

    // layer: request
    auto layer1 = add_transform_layer(begin, end, line_to_request);

    // layer: timestamp >= from
    auto sup_predicate = [from](request b) { return from <= b.get_timestamp(); };
    auto layer2 = add_filter_layer(layer1.first, layer1.second, sup_predicate);

    // layer: timestamp <= to
    auto inf_predicate = [to](request b) { return to >= b.get_timestamp(); };
    auto layer3 = add_filter_layer(layer2.first, layer2.second, inf_predicate);

    return layer3;
}


/*
 * Distinct requests
 *
 * 1) Insert every request in an unordered_set.
 * 2) Print the set size on cout.
 */
template<typename iterator_t>
void compute_distinct(iterator_t begin, iterator_t end)
{
    std::unordered_set<std::string> distinct;
    auto distinct_counter = [&distinct](request& r) { distinct.insert(std::move(r.get_request())); }; // Avoid copies
    std::for_each(begin, end, distinct_counter);
    std::cout << distinct.size() << std::endl;
}


/*
 * N most common requests.
 *
 * 1) Insert every request in an unordered_multiset.
 * 2) Use the average constant count method of the unordered_multiset to find the most frequent requests.
 *    Insert each values of the unordered_multiset in an (ordered) set its count is larger than the smallest count of
 *    the set.
 *    Remove the values with the smallest count in the set when the set size is larger than top_n
 * 3) Print the values in the set to cout.
 */
template<typename iterator_t>
void compute_n_most_common(iterator_t begin, iterator_t end, size_t top_n)
{
    std::unordered_multiset<std::string> freq_set;
    auto insert = [&freq_set](request& r) { freq_set.insert(std::move(r.get_request())); }; // Avoid copies
    std::for_each(begin, end, insert);
    std::set<std::pair<unsigned, std::string>> most_common;
    unsigned worse_n = 0;
    auto top_counter = [&freq_set, &most_common, &worse_n, top_n](const std::string& req) {
        size_t count = freq_set.count(req);
        if (most_common.size() < top_n or count > worse_n)
        {
            most_common.emplace(count, req);
            if (most_common.size() > top_n)
                most_common.erase(*most_common.begin());
            worse_n = most_common.begin()->first;
        }
    };
    std::for_each(freq_set.begin(), freq_set.end(), top_counter);
    for (auto it = most_common.rbegin(); it != most_common.rend(); it++)
        std::cout << it->second << " " << it->first << std::endl;
}


int main(int argc, char* argv[])
{
    size_t from = 0;
    size_t to = UINT64_MAX;
    std::string filename(argv[argc - 1]);
    size_t top_n = 0;

    // Parsing arguments from the command line, the dirty way.
    // The reader can skip to line 346 safely.
    // For this kind of "code", I usually make advantage of a third party lib, but it is not allowed here... T_T
    bool arg_error;
    if (argc < 3)
    {
        std::cerr << "Invalid parameters" << std::endl;
        exit(1);
    }
    std::string mode = argv[1];
    unsigned option_pos = 2;
    if ("distinct" == mode)
    {
        argc -= 3;
        arg_error = argc < 0 || (argc != 2 && argc != 4);
    }
    else if ("top" == mode)
    {
        try
        {
            top_n = std::stoul(argv[2]);
        }
        catch (std::invalid_argument&)
        {
            arg_error = true;
        }
        argc -= 4;
        arg_error = argc < 0 || (argc != 2 && argc != 4 && argc != 0);
        option_pos = 3;
    }
    else
        arg_error = true;
    if (arg_error)
    {
        std::cerr << "Invalid parameters" << std::endl;
        exit(1);
    }

    for (unsigned i = option_pos; i < option_pos + argc; i += 2)
    {
        if (argv[i] == std::string("--from"))
            from = std::stoul(argv[i + 1]);
        else
            to = std::stoul(argv[i + 1]);
    }

    std::ifstream inputFile(filename);
    auto pipeline = define_pipeline(inputFile, from, to);

    if (mode == "top")
        compute_n_most_common(pipeline.first, pipeline.second, top_n);
    else
        compute_distinct(pipeline.first, pipeline.second);

    return 0;
}
