/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>
          (c) 2015 Maxim Zhurovich <zhurovich@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

// Current-friendly container types.

// * (Ordered/Unordered)Dictionary<T> <=> std::(map/unordered_map)<key_t, T>
//   Empty(), Size(), operator[](key), Erase(key) [, iteration, {lower/upper}_bound].
//   `key_t` is either the type of `T.key` or of `T.get_key()`.
//
// * (Ordered/Unordered)Matrix<T> <=> { row_t, col_t } -> T, two `std::(map/unordered_map)<>`-s.
//   Entries are stored in third `std::unordered_map<std::pair<row_t, col_t>, std::unique_ptr<T>>`.
//   Empty(), Size(), Rows()/Cols(), Add(cell), Delete(row, col) [, iteration, {lower/upper}_bound].
//   `row_t` and `col_t` are either the type of `T.row` / `T.col`, or of `T.get_row()` / `T.get_col()`.
//
// All Current-friendly types support persistence.
//
// Only allow default constructors for containers.

#ifndef CURRENT_STORAGE_STORAGE_H
#define CURRENT_STORAGE_STORAGE_H

#include "../port.h"

#include "base.h"
#include "transaction.h"
#include "transaction_policy.h"
#include "transaction_result.h"

#include "container/dictionary.h"
#include "container/matrix.h"
#include "container/one_to_one.h"
#include "container/one_to_many.h"

#include "persister/file.h"

#include "../TypeSystem/struct.h"
#include "../TypeSystem/Serialization/json.h"
#include "../TypeSystem/optional.h"

#include "../Bricks/exception.h"
#include "../Bricks/strings/strings.h"
#include "../Bricks/time/chrono.h"

namespace current {
namespace storage {

#define CURRENT_STORAGE_FIELD_ENTRY_Dictionary_IMPL(dictionary_type, entry_type, entry_name) \
  CURRENT_STRUCT(entry_name##Updated) {                                                      \
    CURRENT_FIELD(data, entry_type);                                                         \
    CURRENT_DEFAULT_CONSTRUCTOR(entry_name##Updated) {}                                      \
    CURRENT_CONSTRUCTOR(entry_name##Updated)(const entry_type& value) : data(value) {}       \
  };                                                                                         \
  CURRENT_STRUCT(entry_name##Deleted) {                                                      \
    CURRENT_FIELD(key, ::current::storage::sfinae::ENTRY_KEY_TYPE<entry_type>);              \
    CURRENT_DEFAULT_CONSTRUCTOR(entry_name##Deleted) {}                                      \
    CURRENT_CONSTRUCTOR(entry_name##Deleted)(const entry_type& value)                        \
        : key(::current::storage::sfinae::GetKey(value)) {}                                  \
  };                                                                                         \
  struct entry_name {                                                                        \
    template <typename T, typename E1, typename E2>                                          \
    using field_t = dictionary_type<T, E1, E2>;                                              \
    using entry_t = entry_type;                                                              \
    using key_t = ::current::storage::sfinae::ENTRY_KEY_TYPE<entry_type>;                    \
    using update_event_t = entry_name##Updated;                                              \
    using delete_event_t = entry_name##Deleted;                                              \
    using persisted_event_1_t = entry_name##Updated;                                         \
    using persisted_event_2_t = entry_name##Deleted;                                         \
    using DEPRECATED_T_(ENTRY) = entry_t;                                                    \
    using DEPRECATED_T_(KEY) = key_t;                                                        \
    using DEPRECATED_T_(UPDATE_EVENT) = update_event_t;                                      \
    using DEPRECATED_T_(DELETE_EVENT) = delete_event_t;                                      \
    using DEPRECATED_T_(PERSISTED_EVENT_1) = persisted_event_1_t;                            \
    using DEPRECATED_T_(PERSISTED_EVENT_2) = persisted_event_2_t;                            \
  }

#define CURRENT_STORAGE_FIELD_ENTRY_UnorderedDictionary(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Dictionary_IMPL(UnorderedDictionary, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY_OrderedDictionary(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Dictionary_IMPL(OrderedDictionary, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY_Matrix_IMPL(matrix_type, entry_type, entry_name)    \
  CURRENT_STRUCT(entry_name##Updated) {                                                 \
    CURRENT_FIELD(data, entry_type);                                                    \
    CURRENT_DEFAULT_CONSTRUCTOR(entry_name##Updated) {}                                 \
    CURRENT_CONSTRUCTOR(entry_name##Updated)(const entry_type& value) : data(value) {}  \
  };                                                                                    \
  CURRENT_STRUCT(entry_name##Deleted) {                                                 \
    CURRENT_FIELD(key,                                                                  \
                  (std::pair<::current::storage::sfinae::ENTRY_ROW_TYPE<entry_type>,    \
                             ::current::storage::sfinae::ENTRY_COL_TYPE<entry_type>>)); \
    CURRENT_DEFAULT_CONSTRUCTOR(entry_name##Deleted) {}                                 \
    CURRENT_CONSTRUCTOR(entry_name##Deleted)(const entry_type& value)                   \
        : key(std::make_pair(::current::storage::sfinae::GetRow(value),                 \
                             ::current::storage::sfinae::GetCol(value))) {}             \
  };                                                                                    \
  struct entry_name {                                                                   \
    template <typename T, typename E1, typename E2>                                     \
    using field_t = matrix_type<T, E1, E2>;                                             \
    using entry_t = entry_type;                                                         \
    using row_t = ::current::storage::sfinae::ENTRY_ROW_TYPE<entry_type>;               \
    using col_t = ::current::storage::sfinae::ENTRY_COL_TYPE<entry_type>;               \
    using key_t = std::pair<row_t, col_t>;                                              \
    using update_event_t = entry_name##Updated;                                         \
    using delete_event_t = entry_name##Deleted;                                         \
    using persisted_event_1_t = entry_name##Updated;                                    \
    using persisted_event_2_t = entry_name##Deleted;                                    \
    using DEPRECATED_T_(ENTRY) = entry_t;                                               \
    using DEPRECATED_T_(ROW) = row_t;                                                   \
    using DEPRECATED_T_(COL) = col_t;                                                   \
    using DEPRECATED_T_(KEY) = key_t;                                                   \
    using DEPRECATED_T_(UPDATE_EVENT) = update_event_t;                                 \
    using DEPRECATED_T_(DELETE_EVENT) = delete_event_t;                                 \
    using DEPRECATED_T_(PERSISTED_EVENT_1) = persisted_event_1_t;                       \
    using DEPRECATED_T_(PERSISTED_EVENT_2) = persisted_event_2_t;                       \
  }

#define CURRENT_STORAGE_FIELD_ENTRY_UnorderedMatrix(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Matrix_IMPL(UnorderedMatrix, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY_OrderedMatrix(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Matrix_IMPL(OrderedMatrix, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY_UnorderedOneToOne(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Matrix_IMPL(UnorderedOneToOne, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY_OrderedOneToOne(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Matrix_IMPL(OrderedOneToOne, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY_UnorderedOneToMany(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Matrix_IMPL(UnorderedOneToMany, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY_OrderedOneToMany(entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_Matrix_IMPL(OrderedOneToMany, entry_type, entry_name)

#define CURRENT_STORAGE_FIELD_ENTRY(container, entry_type, entry_name) \
  CURRENT_STORAGE_FIELD_ENTRY_##container(entry_type, entry_name)

// clang-format on

#define CURRENT_STORAGE_FIELDS_HELPERS(name)                                                                   \
  template <typename T>                                                                                        \
  struct CURRENT_STORAGE_FIELDS_HELPER;                                                                        \
  template <>                                                                                                  \
  struct CURRENT_STORAGE_FIELDS_HELPER<CURRENT_STORAGE_FIELDS_##name<::current::storage::DeclareFields>> {     \
    constexpr static size_t CURRENT_STORAGE_FIELD_INDEX_BASE = __COUNTER__ + 1;                                \
    typedef CURRENT_STORAGE_FIELDS_##name<::current::storage::CountFields> CURRENT_STORAGE_FIELD_COUNT_STRUCT; \
  }

using CURRENT_STORAGE_DEFAULT_PERSISTER_PARAM = persister::NoCustomPersisterParam;

template <typename T>
using CURRENT_STORAGE_DEFAULT_TRANSACTION_POLICY = transaction_policy::Synchronous<T>;

enum class StorageRole : bool { Master = true, Follower = false };

// Generic storage implementation.
template <template <typename...> class PERSISTER,
          typename FIELDS,
          template <typename> class TRANSACTION_POLICY,
          typename CUSTOM_PERSISTER_PARAM>
class GenericStorageImpl {
 public:
  enum { FIELDS_COUNT = ::current::storage::FieldCounter<FIELDS>::value };
  using fields_type_list_t = ::current::storage::FieldsTypeList<FIELDS, FIELDS_COUNT>;
  using fields_variant_t = Variant<fields_type_list_t>;
  using persister_t = PERSISTER<fields_type_list_t, CUSTOM_PERSISTER_PARAM>;
  using DEPRECATED_T_(FIELDS_TYPE_LIST) = fields_type_list_t;
  using DEPRECATED_T_(FIELDS_VARIANT) = fields_variant_t;
  using DEPRECATED_T_(PERSISTER) = persister_t;

 private:
  std::mutex mutex_;
  FIELDS fields_;
  persister_t persister_;
  TRANSACTION_POLICY<persister_t> transaction_policy_;
  StorageRole role_;

 public:
  using fields_by_ref_t = FIELDS&;
  using fields_by_cref_t = const FIELDS&;
  using transaction_t = Transaction<fields_variant_t>;
  using transaction_meta_fields_t = TransactionMetaFields;
  using DEPRECATED_T_(FIELDS_BY_REFERENCE) = fields_by_ref_t;
  using DEPRECATED_T_(FIELDS_BY_CONST_REFERENCE) = fields_by_cref_t;
  using DEPRECATED_T_(TRANSACTION) = transaction_t;
  using DEPRECATED_T_(TRANSACTION_META_FIELDS) = transaction_meta_fields_t;

  GenericStorageImpl(const GenericStorageImpl&) = delete;
  GenericStorageImpl(GenericStorageImpl&&) = delete;
  GenericStorageImpl& operator=(const GenericStorageImpl&) = delete;
  GenericStorageImpl& operator=(GenericStorageImpl&&) = delete;

  template <typename... ARGS>
  GenericStorageImpl(ARGS&&... args)
      : persister_(mutex_,
                   [this](const fields_variant_t& entry) { entry.Call(fields_); },
                   std::forward<ARGS>(args)...),
        transaction_policy_(mutex_, persister_, fields_.current_storage_mutation_journal_) {
    role_ = (persister_.DataAuthority() == persister::PersisterDataAuthority::Own) ? StorageRole::Master
                                                                                   : StorageRole::Follower;
  }

  StorageRole GetRole() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return role_;
  }

  // Used for applying updates by dispatching corresponding events.
  template <typename... ARGS>
  typename std::result_of<FIELDS(ARGS...)>::type operator()(ARGS&&... args) {
    return fields_(std::forward<ARGS>(args)...);
  }

  template <typename F>
  using f_result_t = typename std::result_of<F(fields_by_ref_t)>::type;

  template <typename F>
  ::current::Future<::current::storage::TransactionResult<f_result_t<F>>, ::current::StrictFuture::Strict>
  Transaction(F&& f) {
    return transaction_policy_.Transaction([&f, this]() { return f(fields_); });
  }

  template <typename F1, typename F2>
  ::current::Future<::current::storage::TransactionResult<void>, ::current::StrictFuture::Strict> Transaction(
      F1&& f1, F2&& f2) {
    return transaction_policy_.Transaction([&f1, this]() { return f1(fields_); }, std::forward<F2>(f2));
  }

  void ReplayTransaction(transaction_t&& transaction, ::current::ss::IndexAndTimestamp idx_ts) {
    transaction_policy_.ReplayTransaction([this](fields_variant_t&& entry) {
      entry.Call(fields_);
    }, std::forward<transaction_t>(transaction), idx_ts);
  }

  void ExposeRawLogViaHTTP(int port, const std::string& route) { persister_.ExposeRawLogViaHTTP(port, route); }

  typename std::result_of<decltype(&persister_t::InternalExposeStream)(persister_t)>::type
  InternalExposeStream() {
    return persister_.InternalExposeStream();
  }

  void FlipToMaster() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (role_ == StorageRole::Follower) {
      persister_.AcquireDataAuthority();
      role_ = StorageRole::Master;
    } else {
      CURRENT_THROW(StorageIsAlreadyMasterException());
    }
  }

  void GracefulShutdown() { transaction_policy_.GracefulShutdown(); }
};

#define CURRENT_STORAGE_IMPLEMENTATION(name)                                                                   \
  template <typename INSTANTIATION_TYPE>                                                                       \
  struct CURRENT_STORAGE_FIELDS_##name;                                                                        \
  template <template <typename...> class PERSISTER,                                                            \
            template <typename> class TRANSACTION_POLICY =                                                     \
                ::current::storage::CURRENT_STORAGE_DEFAULT_TRANSACTION_POLICY,                                \
            typename CUSTOM_PERSISTER_PARAM = ::current::storage::CURRENT_STORAGE_DEFAULT_PERSISTER_PARAM>     \
  using name =                                                                                                 \
      ::current::storage::GenericStorageImpl<PERSISTER,                                                        \
                                             CURRENT_STORAGE_FIELDS_##name<::current::storage::DeclareFields>, \
                                             TRANSACTION_POLICY,                                               \
                                             CUSTOM_PERSISTER_PARAM>;                                          \
  CURRENT_STORAGE_FIELDS_HELPERS(name)

// A minimalistic `PERSISTER` which compiles with the above `CURRENT_STORAGE_IMPLEMENTATION` macro.
// Used for the sole purpose of extracting the underlying `transaction_t` type without extra template magic.
namespace persister {

template <typename TYPELIST, template <typename> class UNDERLYING_PERSISTER, typename STREAM_RECORD_TYPE>
class NullStoragePersisterImpl;

template <template <typename> class UNDERLYING_PERSISTER, typename STREAM_RECORD_TYPE, typename... TS>
class NullStoragePersisterImpl<TypeList<TS...>, UNDERLYING_PERSISTER, STREAM_RECORD_TYPE> {
 public:
  using variant_t = Variant<TS...>;
  using transaction_t = Transaction<variant_t>;
  using DEPRECATED_T_(VARIANT) = variant_t;
  using DEPRECATED_T_(TRANSACTION) = transaction_t;

  void InternalExposeStream() {}
};

template <typename>
struct NullPersister {};

template <typename TYPELIST, typename STREAM_RECORD_TYPE = CURRENT_STORAGE_DEFAULT_PERSISTER_PARAM>
using NullStoragePersister = NullStoragePersisterImpl<TYPELIST, NullPersister, STREAM_RECORD_TYPE>;

}  // namespace current::storage::persister

template <template <template <typename...> class, template <typename> class, typename> class STORAGE>
using transaction_t = typename STORAGE<persister::NullStoragePersister,
                                       CURRENT_STORAGE_DEFAULT_TRANSACTION_POLICY,
                                       CURRENT_STORAGE_DEFAULT_PERSISTER_PARAM>::transaction_t;

#define CURRENT_STORAGE(name)            \
  CURRENT_STORAGE_IMPLEMENTATION(name);  \
  template <typename INSTANTIATION_TYPE> \
  struct CURRENT_STORAGE_FIELDS_##name   \
      : ::current::storage::FieldsBase<  \
            CURRENT_STORAGE_FIELDS_HELPER<CURRENT_STORAGE_FIELDS_##name<::current::storage::DeclareFields>>>

// clang-format off
#define CURRENT_STORAGE_FIELD(field_name, entry_name)                                                          \
  using field_container_##field_name##_t = entry_name::field_t<entry_name::entry_t,                            \
                                                                       entry_name::persisted_event_1_t,        \
                                                                       entry_name::persisted_event_2_t>;       \
  using entry_type_##field_name##_t = entry_name;                                                              \
  using field_type_##field_name##_t =                                                                          \
      ::current::storage::Field<INSTANTIATION_TYPE, field_container_##field_name##_t>;                         \
  using DEPRECATED_T_(ENTRY_TYPE_##field_name) = entry_type_##field_name##_t;                                  \
  using DEPRECATED_T_(FIELD_TYPE_##field_name) = field_type_##field_name##_t;                                  \
  using DEPRECATED_T_(FIELD_CONTAINER_TYPE_##field_name) = field_container_##field_name##_t;                   \
  constexpr static size_t FIELD_INDEX_##field_name =                                                           \
      CURRENT_EXPAND_MACRO(__COUNTER__) - CURRENT_STORAGE_FIELD_INDEX_BASE;                                    \
  ::current::storage::FieldInfo<entry_name::persisted_event_1_t, entry_name::persisted_event_2_t> operator()(  \
      ::current::storage::FieldInfoByIndex<FIELD_INDEX_##field_name>) const {                                  \
    return ::current::storage::FieldInfo<entry_name::persisted_event_1_t, entry_name::persisted_event_2_t>();  \
  }                                                                                                            \
  std::string operator()(::current::storage::FieldNameByIndex<FIELD_INDEX_##field_name>) const {               \
    return #field_name;                                                                                        \
  }                                                                                                            \
  const field_type_##field_name##_t& operator()(                                                               \
      ::current::storage::ImmutableFieldByIndex<FIELD_INDEX_##field_name>) const {                             \
    return field_name;                                                                                         \
  }                                                                                                            \
  template <typename F>                                                                                        \
  void operator()(::current::storage::ImmutableFieldByIndex<FIELD_INDEX_##field_name>, F&& f) const {          \
    f(field_name);                                                                                             \
  }                                                                                                            \
  template <typename F, typename RETVAL>                                                                       \
  RETVAL operator()(::current::storage::ImmutableFieldByIndexAndReturn<FIELD_INDEX_##field_name, RETVAL>,      \
                    F&& f) const {                                                                             \
    return f(field_name);                                                                                      \
  }                                                                                                            \
  template <typename F>                                                                                        \
  void operator()(::current::storage::MutableFieldByIndex<FIELD_INDEX_##field_name>, F&& f) {                  \
    f(field_name);                                                                                             \
  }                                                                                                            \
  field_type_##field_name##_t& operator()(::current::storage::MutableFieldByIndex<FIELD_INDEX_##field_name>) { \
    return field_name;                                                                                         \
  }                                                                                                            \
  template <typename F, typename RETVAL>                                                                       \
  RETVAL operator()(::current::storage::MutableFieldByIndexAndReturn<FIELD_INDEX_##field_name, RETVAL>,        \
                    F&& f) {                                                                                   \
    return f(field_name);                                                                                      \
  }                                                                                                            \
  template <typename F>                                                                                        \
  void operator()(::current::storage::FieldNameAndTypeByIndex<FIELD_INDEX_##field_name>, F&& f) const {        \
    f(#field_name,                                                                                             \
      ::current::storage::StorageFieldTypeSelector<field_container_##field_name##_t>(),                        \
      ::current::storage::FieldUnderlyingTypesWrapper<entry_name>());                                          \
  }                                                                                                            \
  ::current::storage::StorageExtractedFieldType<field_container_##field_name##_t> operator()(                  \
      ::current::storage::FieldTypeExtractor<FIELD_INDEX_##field_name>) const {                                \
    return ::current::storage::StorageExtractedFieldType<field_container_##field_name##_t>();                  \
  }                                                                                                            \
  ::current::storage::StorageExtractedFieldType<entry_type_##field_name##_t> operator()(                       \
      ::current::storage::FieldEntryTypeExtractor<FIELD_INDEX_##field_name>) const {                           \
    return ::current::storage::StorageExtractedFieldType<entry_type_##field_name##_t>();                       \
  }                                                                                                            \
  template <typename F, typename RETVAL>                                                                       \
  RETVAL operator()(::current::storage::FieldNameAndTypeByIndexAndReturn<FIELD_INDEX_##field_name, RETVAL>,    \
                    F&& f) const {                                                                             \
    return f(#field_name,                                                                                      \
             ::current::storage::StorageFieldTypeSelector<field_container_##field_name##_t>(),                 \
             ::current::storage::FieldUnderlyingTypesWrapper<entry_name>());                                   \
  }                                                                                                            \
  field_type_##field_name##_t field_name = field_type_##field_name##_t(current_storage_mutation_journal_);     \
  void operator()(const entry_name::persisted_event_1_t& e) { field_name(e); }                                 \
  void operator()(const entry_name::persisted_event_2_t& e) { field_name(e); }
// clang-format on

template <typename STORAGE>
using MutableFields = typename STORAGE::fields_by_ref_t;

template <typename STORAGE>
using ImmutableFields = typename STORAGE::fields_by_cref_t;

template <typename>
struct PerStorageFieldTypeImpl;

template <typename T>
using PerStorageFieldType = PerStorageFieldTypeImpl<typename T::rest_behavior_t>;

template <>
struct PerStorageFieldTypeImpl<rest::behavior::Dictionary> {
  template <typename RECORD>
  static auto ExtractOrComposeKey(const RECORD& entry)
      -> decltype(current::storage::sfinae::GetKey(std::declval<RECORD>())) {
    return current::storage::sfinae::GetKey(entry);
  }
  template <typename DICTIONARY>
  static const DICTIONARY& Iterate(const DICTIONARY& dictionary) {
    return dictionary;
  }
};

template <>
struct PerStorageFieldTypeImpl<rest::behavior::Matrix> {
  template <typename RECORD>
  static auto ExtractOrComposeKey(const RECORD& entry)
      -> std::pair<decltype(current::storage::sfinae::GetRow(std::declval<RECORD>())),
                   decltype(current::storage::sfinae::GetCol(std::declval<RECORD>()))> {
    return std::make_pair(current::storage::sfinae::GetRow(entry), current::storage::sfinae::GetCol(entry));
  }
  template <typename MATRIX>
  struct Iterable {
    const MATRIX& matrix;
    explicit Iterable(const MATRIX& matrix) : matrix(matrix) {}
    using Iterator = typename MATRIX::WholeMatrixIterator;
    Iterator begin() const { return matrix.WholeMatrixBegin(); }
    Iterator end() const { return matrix.WholeMatrixEnd(); }
  };

  template <typename MATRIX>
  static Iterable<MATRIX> Iterate(const MATRIX& matrix) {
    return Iterable<MATRIX>(matrix);
  }
};

}  // namespace current::storage
}  // namespace current

using current::storage::MutableFields;
using current::storage::ImmutableFields;

#endif  // CURRENT_STORAGE_STORAGE_H
