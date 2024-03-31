        // special case for colony as it uses `insert()` instead of `push_back()`
        // and therefore doesn't fit with vector/deque/list
        // for colony of items there is another specialization with RLE
        template < typename T, std::enable_if_t < !std::is_same_v<T, item> > * = nullptr >
        bool read( cata::colony<T> &v, bool throw_on_error = false ) const;

        // object ~> containers with unmatching key_type and value_type
        // map, unordered_map ~> object
        template < typename T, std::enable_if_t <
                       !std::is_same_v<typename T::key_type, typename T::value_type> > * = nullptr
                   >
        bool read( T &m, bool throw_on_error = true ) const;

        [[noreturn]] void string_error( const std::string &message ) const noexcept( false ) {
            string_error( 0, message );
        }

        [[noreturn]] void string_error( int offset, const std::string &message ) const noexcept( false ) {
            JsonPath p;
            if( parent_path_ ) {
                p = *parent_path_ + path_index_;
            }
            string_error( p, offset, message );
        }

        [[noreturn]] void throw_error( const std::string &message ) const noexcept( false ) {
            throw_error( 0, message );
        }

        [[noreturn]] void throw_error( int offset, const std::string &message ) const noexcept( false ) {
            JsonPath p;
            if( parent_path_ ) {
                p = *parent_path_ + path_index_;
            }
            throw_error( p, offset, message );
        }

        [[noreturn]] void throw_error_after( const std::string &message ) const noexcept( false ) {
            JsonPath p;
            if( parent_path_ ) {
                p = *parent_path_ + path_index_;
            }
            throw_error_after( p, message );
        }
    private:
        JsonPath const *parent_path_;
        size_t path_index_;

        // Putting this in the header allows inlining and eliding a bunch of code with constant folding.
        bool error_or_false( bool throw_on_error, const std::string &message ) const noexcept( false ) {
            if( throw_on_error ) {
                throw_error( 0, message );
            }
            return false;
        }
};

class JsonArray : JsonWithPath
{
        static const auto &empty_array_() {
            // NOLINTNEXTLINE(cata-almost-never-auto)
            static auto empty_array = flexbuffer_cache::parse_buffer( "[]" );
            return empty_array;
        }

    public:
        JsonArray() : JsonWithPath( empty_array_(),
                                        flexbuffer_root_from_storage( empty_array_()->get_storage() ), {} ) {
            init( json_ );
        }

        JsonArray(
            std::shared_ptr<parsed_flexbuffer> root,
            flexbuffer json,
            JsonPath &&path )
            : JsonWithPath( std::move( root ), json, std::move( path ) ) {
            init( json_ );
        }

        JsonArray( const JsonArray &rhs ) : JsonWithPath( rhs ) {
            init( json_ );
        }

        JsonArray &operator=( const JsonArray &rhs ) {
            JsonWithPath::operator=( rhs );
            init( json_ );
            return *this;
        }

        JsonArray( JsonArray &&rhs ) noexcept :
            JsonWithPath( std::move( static_cast<JsonWithPath &>( rhs ) ) ) {
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
        }

        JsonArray &operator=( JsonArray &&rhs ) noexcept {
            JsonWithPath::operator=( std::move( static_cast<JsonWithPath &>( rhs ) ) );
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
            return *this;
        }

        class const_iterator;
        friend const_iterator;

        // Iterates the values in this array.
        const_iterator begin() const;
        const_iterator end() const;

        size_t size() const {
            return size_;
        }

        bool empty() const {
            return size() == 0;
        }

        JsonValue operator[]( size_t idx ) const;

        std::string get_string( size_t idx ) const;
        int get_int( size_t idx ) const;
        double get_float( size_t idx ) const;

        JsonArray get_array( size_t idx ) const;
        JsonObject get_object( size_t idx ) const;

        bool has_string( size_t idx ) const;
        bool has_bool( size_t idx ) const;
        bool has_int( size_t idx ) const;
        bool has_float( size_t idx ) const;
        bool has_array( size_t idx ) const;
        bool has_object( size_t idx ) const;

        bool test_string() const;
        std::string next_string();

        bool test_bool() const;
        bool next_bool();

        bool test_int() const;
        int next_int();

        bool test_float() const;
        double next_float();

        bool test_array() const;

        JsonArray next_array();

        bool test_object() const;
        JsonObject next_object();

        JsonValue next_value();

        template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
                 E next_enum_value() ;

                 // array ~> vector, deque, list
                 template < typename T, std::enable_if_t <
                                !std::is_same_v<void, typename T::value_type> > * = nullptr
                            >
                 auto read( T &v, bool throw_on_error = false ) const -> decltype( v.front(), true );

                 // random-access read values by reference
                 template <typename T> bool read_next( T &t, bool throw_on_error = false );

                 // random-access read values by reference
                 template <typename T> bool read( size_t idx, T &t, bool throw_on_error = false ) const;

                 template <typename T = std::string, typename Res = std::set<T>>
                 Res get_tags( size_t idx ) const;

                 [[noreturn]] void string_error( size_t idx, int offset, const std::string &message ) const;

        bool has_more() const {
            return next_ < size_;
        }

        using JsonWithPath::throw_error;
        using JsonWithPath::throw_error_after;
        using JsonWithPath::error_or_false;

    private:
        JsonValue get_next();

        size_t size_ = 0;
        size_t next_ = 0;

        // NOLINTNEXTLINE(cata-large-inline-function)
        void init( const flexbuffer &json, tiny_bitset *moved_visited_fields = nullptr ) noexcept {
            if( json.IsFixedTypedVector() ) {
                size_ = json.AsFixedTypedVector().size();
            } else if( json.IsTypedVector() ) {
                size_ = json.AsTypedVector().size();
            } else {
                size_ = json.AsVector().size();
            }
            if( moved_visited_fields ) {
                using namespace std;
                swap( visited_fields_bitset_, *moved_visited_fields );
            } else {
                visited_fields_bitset_.resize( size_ );
            }
        }

        mutable tiny_bitset visited_fields_bitset_;

        void mark_visited( size_t idx ) const {
            visited_fields_bitset_.set( idx );
        }
};

class JsonMember : public JsonValue
{
    public:
        JsonMember( flexbuffers::String name,
                    JsonValue value ) : JsonValue( std::move( value ) ),
            name_( name ) { }

        JsonMember( const JsonMember & ) = default;
        JsonMember &operator=( const JsonMember & ) = default;

        JsonMember( JsonMember && ) = default;
        JsonMember &operator=( JsonMember && ) = default;

        std::string name() const {
            return name_.str();
        }

        bool is_comment() const {
            return strncmp( name_.c_str(), "//", 2 ) == 0;
        }

    private:
        flexbuffers::String name_;
};

class JsonObject : JsonWithPath
{
    protected:
        flexbuffers::TypedVector keys_ = flexbuffers::TypedVector::EmptyTypedVector();
        flexbuffers::Vector values_ = flexbuffers::Vector::EmptyVector();
        mutable tiny_bitset visited_fields_bitset_;

        static const auto &empty_object_() {
            // NOLINTNEXTLINE(cata-almost-never-auto)
            static auto empty_object = flexbuffer_cache::parse_buffer( "{}" );
            return empty_object;
        }

    public:
        JsonObject() : JsonWithPath( empty_object_(),
                                         flexbuffer_root_from_storage( empty_object_()->get_storage() ), {} ) {
            init( json_ );
        }

        JsonObject(
            std::shared_ptr<parsed_flexbuffer> root,
            flexbuffer json,
            JsonPath &&path
        )
            : JsonWithPath( std::move( root ), json, std::move( path ) ) {
            init( json_ );
        }

        JsonObject( const JsonObject &rhs ) : JsonWithPath( rhs ) {
            init( json_ );
        }

        JsonObject &operator=( const JsonObject &rhs ) {
            JsonWithPath::operator=( rhs );
            init( json_ );
            // Copying an object resets visited fields.
            visited_fields_bitset_.clear_all();
            return *this;
        }

        JsonObject( JsonObject &&rhs ) noexcept :
            JsonWithPath( std::move( static_cast<JsonWithPath &>( rhs ) ) )  {
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
        }

        JsonObject &operator=( JsonObject &&rhs ) noexcept {
            JsonWithPath::operator=( std::move( static_cast<JsonWithPath &>( rhs ) ) );
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
            return *this;
        }

        ~JsonObject();

    private:
        void init( const flexbuffer &json, tiny_bitset *moved_visited_fields = nullptr ) {
            flexbuffers::Map json_map = json.AsMap();
            keys_ = json_map.Keys();
            values_ = json_map.Values();
            if( moved_visited_fields ) {
                using namespace std;
                swap( visited_fields_bitset_, *moved_visited_fields );
            } else {
                visited_fields_bitset_.resize( keys_.size() );
            }
        }

    public:
        size_t size() const {
            return keys_.size();
        }

        std::string get_string( const std::string &key ) const;
        std::string get_string( const char *key ) const;

        template<typename T, typename std::enable_if_t<std::is_convertible_v<T, std::string>>* = nullptr>
        std::string get_string( const std::string &key, T && fallback ) const;

        template<typename T, typename std::enable_if_t<std::is_convertible_v<T, std::string>>* = nullptr>
        std::string get_string( const char *key, T && fallback ) const;

        // Vanilla accessors. Just return the named member and use it's conversion function.
        bool get_bool( std::string_view key ) const;
        int get_int( std::string_view key ) const;
        double get_float( std::string_view key ) const;
        JsonArray get_array( std::string_view key ) const;
        JsonObject get_object( std::string_view key ) const;

        template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
                 E get_enum_value( const std::string &name ) const;
                 template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
                          E get_enum_value( const char *name ) const;

                          template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
                                   E get_enum_value( const std::string &name, E fallback ) const;
                                   template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
                                            E get_enum_value( const char *name, E fallback ) const;

                                            // Sigh.
                                            std::vector<int> get_int_array( std::string_view name ) const;
                                            std::vector<std::string> get_string_array( std::string_view name ) const;
                                            std::vector<std::string> get_as_string_array( const std::string &name ) const;

                                            bool has_member( std::string_view key ) const;
                                            bool has_null( std::string_view key ) const;
                                            bool has_string( std::string_view key ) const;
                                            bool has_bool( std::string_view key ) const;
                                            bool has_number( std::string_view key ) const;
                                            bool has_int( std::string_view key ) const;
                                            bool has_float( std::string_view key ) const;
                                            bool has_array( std::string_view key ) const;
                                            bool has_object( std::string_view key ) const;

                                            // Fallback accessors. Test if the named member exists, and if yes, return it,
                                            // else will return the fallback value. Does *not* test the member is the type
                                            // being requested.
                                            bool get_bool( std::string_view key, bool fallback ) const;
                                            int get_int( std::string_view key, int fallback ) const;
                                            double get_float( std::string_view key, double fallback ) const;

                                            // Tries to get the member, and if found, calls it visited.
                                            std::optional<JsonValue> get_member_opt( std::string_view key ) const;
                                            JsonValue get_member( std::string_view key ) const;
                                            JsonValue operator[]( std::string_view key ) const;

                                            // Schwillions of read overloads
                                            template <typename T>
                                            bool read( std::string_view name, T &t, bool throw_on_error = true ) const;

                                            template <typename T = std::string, typename Res = std::set<T>>
                                            Res get_tags( std::string_view name ) const;

                                            [[noreturn]] void throw_error( const std::string &err ) const;
                                            [[noreturn]] void throw_error_at( std::string_view member, const std::string &err ) const;

        void allow_omitted_members() const {
            visited_fields_bitset_.set_all();
        }

        void copy_visited_members( const JsonObject &rhs ) const {
            visited_fields_bitset_ = rhs.visited_fields_bitset_;
        }

        using Json::str;

        class const_iterator;
        friend const_iterator;

        const_iterator begin() const;
        const_iterator end() const;

    private:
        // Only called by the iterator which can't be manually constructed.
        JsonValue operator[]( size_t idx ) const;

        // NOLINTNEXTLINE(cata-large-inline-function)
        flexbuffers::Reference find_value_ref( const std::string_view key ) const {
            size_t idx = 0;
            bool found = find_map_key_idx( key, keys_, idx );
            if( found ) {
                return values_[ idx ];
            }
            return flexbuffers::Reference();
        }

        // NOLINTNEXTLINE(cata-large-inline-function)
        static bool find_map_key_idx( const std::string_view key, const flexbuffers::TypedVector &keys,
                                      size_t &idx ) {
            // Handlrolled binary search because the STL does not provide a version that just uses indexes.
            std::make_signed_t<size_t>low = 0;
            std::make_signed_t<size_t>high = keys.size() - 1;
            while( low <= high ) {
                std::make_signed_t<size_t> mid = ( high - low ) / 2 + low;

                const std::string_view test_key = keys[ mid ].AsKey();
                int res = string_view_cmp( test_key, key );

                if( res == 0 ) {
                    idx = mid;
                    return true;
                } else if( res < 0 ) {
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }
            return false;
        }

        void mark_visited( size_t idx ) const {
            visited_fields_bitset_.set( idx );
        }

        void report_unvisited() const;

        // Reports an error via JsonObject at this location.
        [[noreturn]] void error_no_member( std::string_view member ) const;

        // debugmsg prints all the skipped members.
        void error_skipped_members( const std::vector<size_t> &skipped_members ) const;
};

// Having this uncommented causes unable to match definition errors for msvc.
// The implementation still exists in the -inl header but has to come after all
// the definitions for JsonValue::read().
//template<typename T>
//void deserialize( std::optional<T> &obj, const JsonValue &jsin );

void add_array_to_set( std::set<std::string> &s, const JsonObject &json,
                       std::string_view name );

#include "flexbuffer_json-inl.h"

#endif // CATA_SRC_FLEXBUFFER_JSON_H
