# Bricks

The [`Bricks`](https://github.com/KnowSheet/Bricks/) repository contains core pieces to be reused across [`KnowSheet`](https://github.com/KnowSheet/) projects.

![](https://raw.githubusercontent.com/KnowSheet/Bricks/master/holy_bricks.jpg)

<sub>Image credit: [Bender](http://en.wikipedia.org/wiki/Bender_(Futurama) from [Futurama](http://en.wikipedia.org/wiki/Futurama). Found via Bing image search.</sub>

<sub>The following documentation has been auto-generated from source code by means of a [`gen-docu.sh`](https://github.com/KnowSheet/KnowSheet/blob/master/scripts/gen-docu.sh) script. Do not edit this file.</sub>

# Release Notes

* *3/4/2015* : **1.0, Initial public release.**

  [Documentation](https://github.com/KnowSheet/Bricks/tree/v1.0#documentation),
  [tar.gz](https://github.com/KnowSheet/Bricks/archive/v1.0.tar.gz),
  [zip](https://github.com/KnowSheet/Bricks/archive/v1.0.zip).

  HTTP server and client API-s, [Cereal](http://uscilab.github.io/cereal/) for binary and JSON serialization, [gnuplot](http://www.gnuplot.info/) and [plotutils](http://www.gnu.org/software/plotutils/) ports, file system and string manipulation methods, a command line flags parsing tool and a header-only port of [GoogleTest](http://code.google.com/p/googletest/).

  Cross-platform, tested on Linux, Mac and Windows. 100% unit tested.

  Header-only C++11 with no external library dependencies.

# Documentation
## Cerealize

Bricks uses [**Cereal**](http://uscilab.github.io/cereal/) for JSON and Binary serialization of C++ objects:

* [Cereal Website](http://uscilab.github.io/cereal/): Cereal is a header-only C++11 serialization library.
* [Cereal GitHub](https://github.com/USCiLab/cereal): A C++11 library for serialization.

<sub>Personal thanks for a well-designed C++11 serialization library! — @dkorolev</sub>

The [`#include "Bricks/cerealize/cerealize.h"`](https://github.com/KnowSheet/Bricks/blob/master/cerealize/cerealize.h) header makes the below code snippets complete.
```cpp
// Add a `serialize()` method to make a C++ structure "cerealizable".
struct SimpleType {
  int number;
  std::string string;
  std::vector<int> vector_int;
  std::map<int, std::string> map_int_string;
  template <typename A> void serialize(A& ar) {
    // Use `CEREAL_NVP(member)` to keep member names when using JSON.
    ar(CEREAL_NVP(number),
       CEREAL_NVP(string),
       CEREAL_NVP(vector_int),
       CEREAL_NVP(map_int_string));
  }
};
```
```cpp
// Use `JSON()` and `ParseJSON()` to create and parse JSON-s.
SimpleType x;
x.number = 42;
x.string = "test passed";
x.vector_int.push_back(1);
x.vector_int.push_back(2);
x.vector_int.push_back(3);
x.map_int_string[1] = "one";
x.map_int_string[42] = "the question";

// `JSON(object)` converts a cerealize-able object into a JSON string.
const std::string json = JSON(x);

// `ParseJSON<T>(json)` creates an instance of T from a JSON.
const SimpleType y = ParseJSON<SimpleType>(json);

// `ParseJSON(json, T& out)` allows omitting the type.
SimpleType z;
ParseJSON(json, z);
```
```cpp
// Use `load()/save()` instead of `serialize()` to customize serialization.
struct LoadSaveType {
  int a;
  int b;
  int sum;

  template <typename A> void save(A& ar) const {
    ar(CEREAL_NVP(a), CEREAL_NVP(b));
  }

  template <typename A> void load(A& ar) {
    ar(CEREAL_NVP(a), CEREAL_NVP(b));
    sum = a + b;
  }
};

LoadSaveType x;
x.a = 2;
x.b = 3;
EXPECT_EQ(5, ParseJSON<LoadSaveType>(JSON(x)).sum);
```
```cpp
// The example below uses `Printf()`, include it.
#include "strings/printf.h"
using bricks::strings::Printf;
 
// Polymorphic types are supported with some caution.
struct ExamplePolymorphicType {
  std::string base;
  explicit ExamplePolymorphicType(const std::string& base = "") : base(base) {}

  virtual std::string AsString() const = 0;
  template <typename A> void serialize(A& ar) {
    ar(CEREAL_NVP(base));
  }
};

struct ExamplePolymorphicInt : ExamplePolymorphicType {
  int i;
  explicit ExamplePolymorphicInt(int i = 0)
      : ExamplePolymorphicType("int"), i(i) {}

  virtual std::string AsString() const override {
    return Printf("%s, %d", base.c_str(), i);
  }

  template <typename A> void serialize(A& ar) {
    ExamplePolymorphicType::serialize(ar);
    ar(CEREAL_NVP(i));
  }
};
// Need to register the derived type.
CEREAL_REGISTER_TYPE(ExamplePolymorphicInt);

struct ExamplePolymorphicDouble : ExamplePolymorphicType {
  double d;
  explicit ExamplePolymorphicDouble(double d = 0)
      : ExamplePolymorphicType("double"), d(d) {}

  virtual std::string AsString() const override {
    return Printf("%s, %lf", base.c_str(), d);
  }

  template <typename A> void serialize(A& ar) {
    ExamplePolymorphicType::serialize(ar);
    ar(CEREAL_NVP(d));
  }
};
// Need to register the derived type.
CEREAL_REGISTER_TYPE(ExamplePolymorphicDouble);

const std::string json_int =
  JSON(WithBaseType<ExamplePolymorphicType>(ExamplePolymorphicInt(42)));

const std::string json_double =
  JSON(WithBaseType<ExamplePolymorphicType>(ExamplePolymorphicDouble(M_PI)));

EXPECT_EQ("int, 42",
          ParseJSON<std::unique_ptr<ExamplePolymorphicType>>(json_int)->AsString());

EXPECT_EQ("double, 3.141593",
          ParseJSON<std::unique_ptr<ExamplePolymorphicType>>(json_double)->AsString());
```
## REST API Toolkit

The [`#include "Bricks/net/api/api.h"`](https://github.com/KnowSheet/Bricks/blob/master/net/api/api.h) header enables to run the code snippets below.

### HTTP Client

```cpp
// Simple GET.
EXPECT_EQ("OK", HTTP(GET("http://test.tailproduce.org/ok")).body);

// More fields.
const auto response = HTTP(GET("http://test.tailproduce.org/ok"));
EXPECT_EQ("OK", response.body);
EXPECT_TRUE(response.code == HTTPResponseCode.OK);
```
```cpp
// POST is supported as well.
EXPECT_EQ("OK", HTTP(POST("http://test.tailproduce.org/ok"), "BODY", "text/plain").body);

// Beyond plain strings, cerealizable objects can be passed in.
// JSON will be sent, as "application/json" content type.
EXPECT_EQ("OK", HTTP(POST("http://test.tailproduce.org/ok"), SimpleType()).body);

```
HTTP client supports headers, POST-ing data to and from files, and many other features as well. Check the unit test in [`bricks/net/api/test.cc`](https://github.com/KnowSheet/Bricks/blob/master/net/api/test.cc) for more details.
### HTTP Server
```cpp
// Simple "OK" endpoint.
HTTP(port).Register("/ok", [](Request r) {
  r("OK");
});
```
```cpp
// Accessing input fields.
HTTP(port).Register("/demo", [](Request r) {
  r(r.url.query["q"] + ' ' + r.method + ' ' + r.body);
});
```
```cpp
// Constructing a more complex response.
HTTP(port).Register("/found", [](Request r) {
  r("Yes.",
    HTTPResponseCode.Accepted,
    "text/html",
    HTTPHeaders().Set("custom", "header").Set("another", "one"));
});
```
```cpp
// An input record that would be passed in as a JSON.
struct PennyInput {
  std::string op;
  std::vector<int> x;
  std::string error;  // Not serialized.
  template <typename A> void serialize(A& ar) {
    ar(CEREAL_NVP(op), CEREAL_NVP(x));
  }
  void FromInvalidJSON(const std::string& input_json) {
    error = "JSON parse error: " + input_json;
  }
};

// An output record that would be sent back as a JSON.
struct PennyOutput {
  std::string error;
  int result;
  template <typename A> void serialize(A& ar) {
    ar(CEREAL_NVP(error), CEREAL_NVP(result));
  }
}; 

// Doing Penny-level arithmetics for fun and performance testing.
HTTP(port).Register("/penny", [](Request r) {
  const auto input = ParseJSON<PennyInput>(r.body);
  if (!input.error.empty()) {
    r(PennyOutput{input.error, 0});
  } else {
    if (input.op == "add") {
      if (!input.x.empty()) {
        int result = 0;
        for (const auto v : input.x) {
          result += v;
        }
        r(PennyOutput{"", result});
      } else {
        r(PennyOutput{"Not enough arguments for 'add'.", 0});
      }
    } else if (input.op == "mul") {
      if (!input.x.empty()) {
        int result = 1;
        for (const auto v : input.x) {
          result *= v;
        }
        r(PennyOutput{"", result});
      } else {
        r(PennyOutput{"Not enough arguments for 'mul'.", 0});
      }
    } else {
      r(PennyOutput{"Unknown operation: " + input.op, 0});
    }
  }
});
```
```cpp
// Returning a potentially unlimited response chunk by chunk.
HTTP(port).Register("/chunked", [](Request r) {
  const size_t n = atoi(r.url.query["n"].c_str());
  const size_t delay_ms = atoi(r.url.query["delay_ms"].c_str());
    
  const auto sleep = [&delay_ms]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
  };
    
  auto response = r.SendChunkedResponse();

  sleep();
  for (size_t i = 0; n && i < n; ++i) {
    response(".");  // Use double quotes for a string, not single quotes for a char.
    sleep();
  }
  response("\n");
});

EXPECT_EQ(".....\n", HTTP(GET("http://test.tailproduce.org/chunked?n=5&delay_ms=2")).body);

// NOTE: For most legitimate practical usecases of returning unlimited
// amounts of data, consider Sherlock's stream data replication mechanisms.
// TODO(dkorolev): Check in Sherlock into KnowSheet.
```
HTTP server also has support for several other features, check out the [`bricks/net/api/test.cc`](https://github.com/KnowSheet/Bricks/blob/master/net/api/test.cc) unit test.
## Visualization Library

Bricks has C++ bindings for [`gnuplot`](http://www.gnuplot.info/), [`#include "Bricks/graph/gnuplot.h"`](https://github.com/KnowSheet/Bricks/blob/master/graph/gnuplot.h) to use it.

External [`gnuplot`](http://www.gnuplot.info/) binary is invoked. The requirement is that it should be installed in the system and accessible in the `$PATH`.
```cpp
// Where visualization meets love.
using namespace bricks::gnuplot;
const size_t image_dim = 800;
const std::string result = GNUPlot()
  .Title("Imagine all the people ...")
  .NoKey()
  .Grid("back")
  .XLabel("... living life in peace")
  .YLabel("John Lennon, \"Imagine\"")
  .Plot(WithMeta([](Plotter p) {
    const size_t N = 1000;
    for (size_t i = 0; i < N; ++i) {
      const double t = M_PI * 2 * i / (N - 1);
      p(16 * pow(sin(t), 3),
        -(13 * cos(t) + 5 * cos(t * 2) - 2 * cos(t * 3) - cos(t * 4)));
    }
  }).LineWidth(5).Color("rgb '#FF0080'"))
  .ImageSize(image_dim)
  .OutputFormat("svg");  // Although the one below is actually a "png".
```
![](https://raw.githubusercontent.com/dkorolev/Bricks/master/graph/golden/love-Linux.png)
```cpp
// Where visualization meets science.
using namespace bricks::gnuplot;
const size_t image_dim = 800;
const std::string result = GNUPlot()
  .Title("Graph 'title' with various \"quotes\"")
  .KeyTitle("'Legend', also known as the \"key\"")
  .XRange(-5, +5)
  .YRange(-2, +2)
  .Grid("back")
  .Plot([](Plotter& p) {
    for (int i = -50; i <= +50; ++i) {
      p(0.1 * i, ::sin(0.1 * i));
    }
  })
  .Plot(WithMeta([](Plotter& p) {
                   for (int i = -50; i <= +50; ++i) {
                     p(0.1 * i, ::cos(0.1 * i));
                   }
                 })
            .AsPoints()
            .Color("rgb 'blue'")
            .Name("\"cos(x)\", '.AsPoints().Color(\"rgb 'blue'\")'"))
  .ImageSize(image_dim)
  .OutputFormat("svg");  // Although the one below is actually a "png".
```
![](https://raw.githubusercontent.com/dkorolev/Bricks/master/graph/golden/science-Linux.png)
```cpp
#include "../../strings/printf.h"

// Show labels on the plane.
using namespace bricks::gnuplot;
const size_t image_dim = 800;
const std::string result = GNUPlot()
  .Title("Labeled Points")
  .NoKey()
  .NoTics()
  .NoBorder()
  .Grid("back")
  .XRange(-1.5, +1.5)
  .YRange(-1.5, +1.5)
  .Plot(WithMeta([](Plotter& p) {
    const int N = 7;
    for (int i = 0; i < N; ++i) {
      const double phi = M_PI * 2 * i / N;
      p(cos(phi), sin(phi), bricks::strings::Printf("P%d", i));
    }
  }).AsLabels())
  .ImageSize(image_dim)
  .OutputFormat("svg");  // Although the one below is actually a "png".
```
![](https://raw.githubusercontent.com/dkorolev/Bricks/master/graph/golden/labels-Linux.png)
## Functional Template Library

Bricks makes extensive use of C++11 variadic templates. A few generic methods are exposed as `bricks::variadic`.
```cpp
// Map.
template<typename T> struct add_100 { enum { x = T::x + 100 }; };

struct A { enum { x = 1 }; };
struct B { enum { x = 2 }; };
struct C { enum { x = 3 }; };

std::tuple<A, B, C> before;
static_assert(std::tuple_size<decltype(before)>::value == 3, "");
EXPECT_EQ(1, std::get<0>(before).x);
EXPECT_EQ(2, std::get<1>(before).x);
EXPECT_EQ(3, std::get<2>(before).x);

bricks::variadic::map<add_100, decltype(before)> after;
static_assert(std::tuple_size<decltype(after)>::value == 3, "");
EXPECT_EQ(101, std::get<0>(after).x);
EXPECT_EQ(102, std::get<1>(after).x);
EXPECT_EQ(103, std::get<2>(after).x);

// Filter.
template <typename T> struct y_is_even { enum { filter = ((T::y % 2) == 0) }; };

struct A { enum { y = 10 }; };
struct B { enum { y = 15 }; };
struct C { enum { y = 20 }; };

std::tuple<A, B, C> before;
static_assert(std::tuple_size<decltype(before)>::value == 3, "");
EXPECT_EQ(10, std::get<0>(before).y);
EXPECT_EQ(15, std::get<1>(before).y);
EXPECT_EQ(20, std::get<2>(before).y);

bricks::variadic::filter<y_is_even, decltype(before)> after;
static_assert(std::tuple_size<decltype(after)>::value == 2, "");
EXPECT_EQ(10, std::get<0>(after).y);
EXPECT_EQ(20, std::get<1>(after).y);
  
// Reduce.
template<typename A, typename B> struct concatenate_s {
  static std::string s() { return "(" + A::s() + "+" + B::s() + ")"; }
};
    
struct A { static std::string s() { return "A"; } };
struct B { static std::string s() { return "B"; } };
struct C { static std::string s() { return "C"; } };
EXPECT_EQ("(A+(B+C))",
          (bricks::variadic::reduce<concatenate_s, std::tuple<A,B,C>>::s()));
  
// Combine.
struct A { static std::string foo() { return "foo"; } };
struct B { static std::string bar() { return "bar"; } };
struct C { static std::string baz() { return "baz"; } };

bricks::variadic::combine<std::tuple<A,B,C>> c;

EXPECT_EQ("foo", c.foo());
EXPECT_EQ("bar", c.bar());
EXPECT_EQ("baz", c.baz());
```
## Run-Time Type Dispatching

Bricks can dispatch calls to the right implementation at runtime, with user code being free of virtual functions.

This comes especially handy when processing log entries from a large stream of data, where only a few types are of immediate interest.

Use the [`#include "Bricks/rtti/dispatcher.h"`](https://github.com/KnowSheet/Bricks/blob/master/rtti/dispatcher.h) header to run the code snippets below.

`TODO(dkorolev)` a wiser way for the end user to leverage the above is by means of `Sherlock` once it's checked in.
```cpp
// The example below uses `Printf()`, include it.
#include "strings/printf.h"
using bricks::strings::Printf;
 
struct ExampleBase {
  virtual ~ExampleBase() = default;
};

struct ExampleInt : ExampleBase {
  int i;
  explicit ExampleInt(int i) : i(i) {}
};

struct ExampleString : ExampleBase {
  std::string s;
  explicit ExampleString(const std::string& s) : s(s) {}
};

struct ExampleMoo : ExampleBase {
};

struct ExampleProcessor {
  std::string result;
  void operator()(const ExampleBase&) { result = "unknown"; }
  void operator()(const ExampleInt& x) { result = Printf("int %d", x.i); }
  void operator()(const ExampleString& x) { result = Printf("string '%s'", x.s.c_str()); }
  void operator()(const ExampleMoo&) { result = "moo!"; }
};

using bricks::rtti::RuntimeTupleDispatcher;
typedef RuntimeTupleDispatcher<ExampleBase,
                               tuple<ExampleInt, ExampleString, ExampleMoo>> Dispatcher;

ExampleProcessor processor;

Dispatcher::DispatchCall(ExampleBase(), processor);
EXPECT_EQ(processor.result, "unknown");

Dispatcher::DispatchCall(ExampleInt(42), processor);
EXPECT_EQ(processor.result, "int 42");

Dispatcher::DispatchCall(ExampleString("foo"), processor);
EXPECT_EQ(processor.result, "string 'foo'");

Dispatcher::DispatchCall(ExampleMoo(), processor);
EXPECT_EQ(processor.result, "moo!");
```
## Command Line Parsing: `dflags`

Bricks has [`dflags`](https://github.com/KnowSheet/Bricks/blob/master/dflags/dflags.h): a C++ library to parse command-line flags.

```cpp
DEFINE_int32(answer, 42, "Human-readable flag description.");
DEFINE_string(question, "six by nine", "Another human-readable flag description.");

void example() {
  std::cout << FLAGS_question.length() << ' ' << FLAGS_answer * FLAGS_answer << std::endl;
}

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);
  // `google::ParseCommandLineFlags(&argc, &argv);`
  // is supported as well for compatibility reasons.
  example();
}
```

Supported types are `string` as `std::string`, `int32`, `uint32`, `int64`, `uint64`, `float`, `double` and `bool`. Booleans accept `0`/`1` and lowercase or capitalized `true`/`false`/`yes`/`no`.

Flags can be passed in as `-flag=value`, `--flag=value`, `-flag value` or `--flag value` parameters.

Undefined flag triggers an error message dumped into stderr followed by exit(-1).  Same happens if `ParseDFlags()` was called more than once.

Non-flag parameters are kept; ParseDFlags() replaces argc/argv with the new, updated values, eliminating the ones holding the parsed flags. In other words `./main foo --flag_bar=bar baz` results in new `argc == 2`, new `argv == { argv[0], "foo", "baz" }`.

Passing `--help` will cause `ParseDFlags()` to print all registered flags with their descriptions and `exit(0)`.

[`dflags`](https://github.com/KnowSheet/Bricks/blob/master/dflags/dflags.h) is a simplified header-only version of Google's [`gflags`](https://code.google.com/p/gflags/). It requires no linker dependencies and largely is backwards-compatible.
## Unit Testing: `gtest`

Bricks contains a header-only port of Google's [`GoogleTest`](http://code.google.com/p/googletest/): a great C++ unit testing library open-sourced by [**Google**](https://www.google.com/finance?q=GOOG).

Check out [`Bricks/file/test.cc`](https://github.com/KnowSheet/Bricks/blob/master/file/test.cc), or most of other `test.cc` files in `Bricks` for example usage.

A three-minute intro:

1. **Logic**
  
  `ASSERT_*` interrupts the `TEST() { .. }` if failing,
  
   `EXPECT_*` considers the `TEST()` failed, but continues to execute it.

2. **Debug Output**
  
   `ASSERT`-s and `EXPECT`-s can be used as output streams. No newline needed.
  
   `EXPECT_EQ(4, 2 * 2) << "Something is wrong with multiplication.";`

3. **Conditions**
  
   `ASSERT`-s and `EXPECT`-s can use {`EQ`,`NE`,`LT`,`GT`,`LE`,`NE`,`TRUE`} after the underscore.
  
   This results in more meaningful human-readable test failure messages.

4. **Parameters Order**
  
   For `{ASSERT,EXPECT}_{EQ,NE}`, put the expected value as the first parameter.
  
   For clean error messages wrt `expected` vs. `actual`.
   

5. **Exceptions**
  
   `ASSERT_THROW(statement, exception_type);` ensures the exception is thrown.

6. **Death Tests**

   The following contsruct:
  
   `ASSERT_DEATH(function(), "Expected regex for the last line of standard error.");`
  
   can be used to ensure certain call fails. The failure implies the binary terminating with a non-zero exit code. The convention is to use the `"DeathTest"` suffix for those tests and to not mix functional tests with death tests.

7. **Templated Tests**
  
   `gtest` supports templated tests, where objects of various tests are passed to the same test method.
  
   Each type results in the whole new statically compiled test.

8. **Disabled Tests**
  
   Prefix a test name with `"DISABLED_"` to exclude it from being run.
  
   Use sparingly and try to keep master clean from disabled tests.

For more details please refer to the original [`GoogleTest` documentation](http://code.google.com/p/googletest/wiki/Documentation).

The code in Bricks is a header-only port of the code originally released by Google. It requires no linker dependencies.
## Extras

[`Bricks`](https://github.com/KnowSheet/Bricks/) contains several other useful bits, including cross-platform file system wrapper, string manipulation functions, in-memory message queue and system clock utilities.
