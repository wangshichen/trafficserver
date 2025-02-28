/*
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
 * These are misc unit tests for uri signing
 */

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

extern "C" {
#include <jansson.h>
#include <cjose/cjose.h>
#include "../jwt.h"
#include "../normalize.h"
#include "../parse.h"
#include "../match.h"
#include "../config.h"
}

static char const *const testConfig =
  R"(
{
    "Master Issuer": {
        "renewal_kid": "6",
        "id": "tester",
        "auth_directives": [
            {
                "auth": "allow",
                "uri": "regex:invalid"
            }
        ],
        "keys": [
         {
          "alg": "HS256",
          "k": "nxb7fyO5Z2hGz9E3oKm1357ptvC2su5QwQUb4YaIaIc",
          "kid": "0",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "cXKukBqFvQ0n3WAuRnWfExC14dmHdGoJULoZjGu9tJC",
          "kid": "1",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "38pJlSXfX87jWL0a03luml9QzUmM4qts1nmfIHA3B7r",
          "kid": "2",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "zNQPphknDGvzR5kA7IonXIDWKMyB1b8NpGmmDNlpgtM",
          "kid": "3",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "iB2ogCmQRt7r5hW7pgyP5FqiFcCl53MPQvfXv8wrZAn",
          "kid": "4",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "GJMCTyZhNoSOZvUOKmmY9MtGSLaONNLHqtKwsC3MWKo",
          "kid": "5",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "u2LziZKJFBnOfjUQUmvot7C9t91jj7ocJPIU9aDdbUl",
          "kid": "6",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "DRBKrBh87NYkH3UzfW1tWbiXCYXiYGZUE9w1orZngL0",
          "kid": "7",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "KNNKFbun8lEs7GbiKlo9mYGNdvpt33tdFzHbNnasDyP",
          "kid": "8",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "yb6kOddMUdupPRSkWMUdE6jrWT4MqUnVyTjpeJBYIqp",
          "kid": "9",
          "kty": "oct"
         }
        ]
    },
    "Second Issuer": {
        "keys": [
         {
          "alg": "HS256",
          "k": "testkey1",
          "kid": "one",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "testkey2",
          "kid": "two",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "testkey3",
          "kid": "three",
          "kty": "oct"
         },
         {
          "alg": "HS256",
          "k": "testkey4",
          "kid": "four",
          "kty": "oct"
         }
        ]
    }
}
)";

bool
jwt_parsing_helper(const char *jwt_string)
{
  fprintf(stderr, "Parsing JWT from string: %s\n", jwt_string);
  bool resp;
  json_error_t jerr             = {};
  size_t pt_ct                  = strlen(jwt_string);
  struct json_t *const jwk_json = json_loadb(jwt_string, pt_ct, 0, &jerr);
  if (!jwk_json) {
    return false;
  }

  struct jwt *jwt = parse_jwt(jwk_json);
  if (!jwt) {
    json_decref(jwk_json);
    return false;
  }

  resp = jwt_validate(jwt);
  jwt_delete(jwt);
  return resp;
}

bool
normalize_uri_helper(const char *uri, const char *expected_normal)
{
  size_t uri_ct = strlen(uri);
  int buff_size = uri_ct + 2;
  int err;
  char *uri_normal = static_cast<char *>(malloc(buff_size));
  memset(uri_normal, 0, buff_size);

  err = normalize_uri(uri, uri_ct, uri_normal, buff_size);

  if (err) {
    free(uri_normal);
    return false;
  }

  if (expected_normal && strcmp(expected_normal, uri_normal) == 0) {
    free(uri_normal);
    return true;
  }

  free(uri_normal);
  return false;
}

bool
remove_dot_helper(const char *path, const char *expected_path)
{
  fprintf(stderr, "Removing Dot Segments from Path: %s\n", path);
  size_t path_ct = strlen(path);
  path_ct++;
  int new_ct;
  char path_buffer[path_ct];
  memset(path_buffer, 0, path_ct);

  new_ct = remove_dot_segments(path, path_ct, path_buffer, path_ct);

  if (new_ct < 0) {
    return false;
  } else if (strcmp(expected_path, path_buffer) == 0) {
    return true;
  } else {
    return false;
  }
}

bool
jws_parsing_helper(const char *uri, const char *paramName, const char *expected_strip)
{
  bool resp;
  size_t uri_ct   = strlen(uri);
  size_t strip_ct = 0;

  char *uri_strip = static_cast<char *>(malloc(uri_ct + 1));
  memset(uri_strip, 0, uri_ct + 1);

  cjose_jws_t *jws = get_jws_from_uri(uri, uri_ct, paramName, uri_strip, uri_ct, &strip_ct);
  if (jws) {
    resp = true;
    if (strcmp(uri_strip, expected_strip) != 0) {
      cjose_jws_release(jws);
      resp = false;
    }
  } else {
    resp = false;
  }
  cjose_jws_release(jws);
  free(uri_strip);
  return resp;
}

TEST_CASE("1", "[JWSParsingTest]")
{
  INFO("TEST 1, Test JWT Parsing From Token Strings");

  SECTION("Standard JWT Parsing")
  {
    REQUIRE(jwt_parsing_helper("{\"cdniets\":30,\"cdnistt\":1,\"exp\":7284188499,\"iss\":\"Content Access "
                               "Manager\",\"cdniuc\":\"uri-regex:http://foobar.local/testDir/*\"}"));
  }

  SECTION("JWT Parsing With Unknown Claim")
  {
    REQUIRE(jwt_parsing_helper("{\"cdniets\":30,\"cdnistt\":1,\"exp\":7284188499,\"iss\":\"Content Access "
                               "Manager\",\"cdniuc\":\"uri-regex:http://foobar.local/testDir/"
                               "*\",\"jamesBond\":\"Something,Something_else\"}"));
  }

  SECTION("JWT Parsing with unsupported crit claim passed")
  {
    REQUIRE(!jwt_parsing_helper("{\"cdniets\":30,\"cdnistt\":1,\"exp\":7284188499,\"iss\":\"Content Access "
                                "Manager\",\"cdniuc\":\"uri-regex:http://foobar.local/testDir/"
                                "*\",\"cdnicrit\":\"Something,Something_else\"}"));
  }

  SECTION("JWT Parsing with empty exp claim")
  {
    REQUIRE(jwt_parsing_helper("{\"cdniets\":30,\"cdnistt\":1,\"iss\":\"Content Access "
                               "Manager\",\"cdniuc\":\"uri-regex:http://foobar.local/testDir/*\"}"));
  }

  SECTION("JWT Parsing with unsupported cdniip claim")
  {
    REQUIRE(!jwt_parsing_helper("{\"cdniets\":30,\"cdnistt\":1,\"cdniip\":\"123.123.123.123\",\"iss\":\"Content Access "
                                "Manager\",\"cdniuc\":\"uri-regex:http://foobar.local/testDir/*\"}"));
  }

  SECTION("JWT Parsing with unsupported value for cdnistd claim")
  {
    REQUIRE(!jwt_parsing_helper("{\"cdniets\":30,\"cdnistt\":1,\"cdnistd\":-2,\"iss\":\"Content Access "
                                "Manager\",\"cdniuc\":\"uri-regex:http://foobar.local/testDir/*\"}"));
  }
  fprintf(stderr, "\n");
}

TEST_CASE("2", "[JWSFromURLTest]")
{
  INFO("TEST 2, Test JWT Parsing and Stripping From URLs");

  SECTION("Token at end of URI")
  {
    REQUIRE(jws_parsing_helper(
      "www.foo.com/hellothere/"
      "URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c",
      "URISigningPackage", "www.foo.com/hellothere"));
  }

  SECTION("No Token in URL")
  {
    REQUIRE(!jws_parsing_helper(
      "www.foo.com/hellothere/"
      "URISigningPackag=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c",
      "URISigningPackage", NULL));
  }

  SECTION("Token in middle of the URL")
  {
    REQUIRE(jws_parsing_helper("www.foo.com/hellothere/"
                               "URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c/Something/Else",
                               "URISigningPackage", "www.foo.com/hellothere/Something/Else"));
  }

  SECTION("Token at the start of the URL")
  {
    REQUIRE(jws_parsing_helper(":URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c/www.foo.com/hellothere/Something/Else",
                               "URISigningPackage", "/www.foo.com/hellothere/Something/Else"));
  }

  SECTION("Pass empty path parameter at end")
  {
    REQUIRE(!jws_parsing_helper("www.foobar.com/hellothere/URISigningPackage=", "URISigningPackage", NULL));
  }

  SECTION("Pass empty path parameter in the middle of URL")
  {
    REQUIRE(!jws_parsing_helper("www.foobar.com/hellothere/URISigningPackage=/Something/Else", "URISigningPackage", NULL));
  }

  SECTION("Partial package name in previous path parameter")
  {
    REQUIRE(jws_parsing_helper("www.foobar.com/URISig/"
                               "URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c/Something/Else",
                               "URISigningPackage", "www.foobar.com/URISig/Something/Else"));
  }

  SECTION("Package comes directly after two reserved characters")
  {
    REQUIRE(jws_parsing_helper("www.foobar.com/"
                               ":URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c/Something/Else",
                               "URISigningPackage", "www.foobar.com//Something/Else"));
  }

  SECTION("Package comes directly after string of reserved characters")
  {
    REQUIRE(jws_parsing_helper("www.foobar.com/?!/"
                               ":URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c/Something/Else",
                               "URISigningPackage", "www.foobar.com/?!//Something/Else"));
  }

  SECTION("Invalid token passed before a valid token")
  {
    REQUIRE(!jws_parsing_helper("www.foobar.com/URISigningPackage=/"
                                "URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                                "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                                "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c/Something/Else",
                                "URISigningPackage", NULL));
  }

  SECTION("Empty string as URL")
  {
    REQUIRE(!jws_parsing_helper("", "URISigningPackage", NULL));
  }

  SECTION("Empty package name to parser")
  {
    REQUIRE(!jws_parsing_helper(
      "www.foobar.com/"
      "URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c",
      "", NULL));
  }

  SECTION("Custom package name with a reserved character - at the end of the URI")
  {
    REQUIRE(jws_parsing_helper(
      "www.foobar.com/CustomPackage/"
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
      "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c",
      "CustomPackage/", "www.foobar.com"));
  }

  SECTION("Custom package name with a reserved character - in the middle of the URI")
  {
    REQUIRE(jws_parsing_helper(
      "www.foobar.com/CustomPackage/"
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
      "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c/Something/Else",
      "CustomPackage/", "www.foobar.com/Something/Else"));
  }

  SECTION("URI signing package passed as the only a query parameter")
  {
    REQUIRE(jws_parsing_helper(
      "www.foobar.com/Something/"
      "Here?URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c",
      "URISigningPackage", "www.foobar.com/Something/Here"));
  }

  SECTION("URI signing package passed as first of many query parameters")
  {
    REQUIRE(jws_parsing_helper("www.foobar.com/Something/"
                               "Here?URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c&query3=foobar&query1=foo&query2=bar",
                               "URISigningPackage", "www.foobar.com/Something/Here?query3=foobar&query1=foo&query2=bar"));
  }

  SECTION("URI signing package passed as one of many query parameters - passed in middle")
  {
    REQUIRE(jws_parsing_helper("www.foobar.com/Something/"
                               "Here?query1=foo&query2=bar&URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c&query3=foobar",
                               "URISigningPackage", "www.foobar.com/Something/Here?query1=foo&query2=bar&query3=foobar"));
  }

  SECTION("URI signing package passed as last of many query parameters")
  {
    REQUIRE(jws_parsing_helper("www.foobar.com/Something/"
                               "Here?query1=foo&query2=bar&URISigningPackage=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                               "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ."
                               "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c",
                               "URISigningPackage", "www.foobar.com/Something/Here?query1=foo&query2=bar"));
  }
}

TEST_CASE("3", "[RemoveDotSegmentsTest]")
{
  INFO("TEST 3, Test Removal of Dot Segments From Paths");

  SECTION("../bar test")
  {
    REQUIRE(remove_dot_helper("../bar", "bar"));
  }

  SECTION("./bar test")
  {
    REQUIRE(remove_dot_helper("./bar", "bar"));
  }

  SECTION(".././bar test")
  {
    REQUIRE(remove_dot_helper(".././bar", "bar"));
  }

  SECTION("./../bar test")
  {
    REQUIRE(remove_dot_helper("./../bar", "bar"));
  }

  SECTION("/foo/./bar test")
  {
    REQUIRE(remove_dot_helper("/foo/./bar", "/foo/bar"));
  }

  SECTION("/bar/./ test")
  {
    REQUIRE(remove_dot_helper("/bar/./", "/bar/"));
  }

  SECTION("/. test")
  {
    REQUIRE(remove_dot_helper("/.", "/"));
  }

  SECTION("/bar/. test")
  {
    REQUIRE(remove_dot_helper("/bar/.", "/bar/"));
  }

  SECTION("/foo/../bar test")
  {
    REQUIRE(remove_dot_helper("/foo/../bar", "/bar"));
  }

  SECTION("/bar/../ test")
  {
    REQUIRE(remove_dot_helper("/bar/../", "/"));
  }

  SECTION("/.. test")
  {
    REQUIRE(remove_dot_helper("/..", "/"));
  }

  SECTION("/bar/.. test")
  {
    REQUIRE(remove_dot_helper("/bar/..", "/"));
  }

  SECTION("/foo/bar/.. test")
  {
    REQUIRE(remove_dot_helper("/foo/bar/..", "/foo/"));
  }

  SECTION("Single . test")
  {
    REQUIRE(remove_dot_helper(".", ""));
  }

  SECTION("Single .. test")
  {
    REQUIRE(remove_dot_helper("..", ""));
  }

  SECTION("Test foo/bar/.. test")
  {
    REQUIRE(remove_dot_helper("foo/bar/..", "foo/"));
  }

  SECTION("Test Empty Path Segment")
  {
    REQUIRE(remove_dot_helper("", ""));
  }

  SECTION("Test mixed operations")
  {
    REQUIRE(remove_dot_helper("/foo/bar/././something/../foobar", "/foo/bar/foobar"));
  }
  fprintf(stderr, "\n");
}

TEST_CASE("4", "[NormalizeTest]")
{
  INFO("TEST 4, Test Normalization of URIs");

  SECTION("Testing passing too small of a URI to normalize")
  {
    REQUIRE(!normalize_uri_helper("ht", NULL));
  }

  SECTION("Testing passing non http/https protocol")
  {
    REQUIRE(!normalize_uri_helper("ht:", NULL));
  }

  SECTION("Passing a uri with half encoded value at end")
  {
    REQUIRE(!normalize_uri_helper("http://www.foobar.co%4", NULL));
  }

  SECTION("Passing a uri with half encoded value in the middle")
  {
    REQUIRE(!normalize_uri_helper("http://www.foobar.co%4psomethin/Path", NULL));
  }

  SECTION("Passing a uri with an empty path parameter")
  {
    REQUIRE(normalize_uri_helper("http://www.foobar.com", "http://www.foobar.com/"));
  }

  SECTION("Passing a uri with an empty path parameter and additional query params")
  {
    REQUIRE(normalize_uri_helper("http://www.foobar.com?query1=foo&query2=bar", "http://www.foobar.com/?query1=foo&query2=bar"));
  }

  SECTION("Empty path parameter with port")
  {
    REQUIRE(normalize_uri_helper("http://www.foobar.com:9301?query1=foo&query2=bar",
                                 "http://www.foobar.com:9301/?query1=foo&query2=bar"));
  }

  SECTION("Passing a uri with a username and password")
  {
    REQUIRE(normalize_uri_helper("http://foo%40:PaSsword@www.Foo%42ar.coM:80/", "http://foo%40:PaSsword@www.foobar.com/"));
  }

  SECTION("Testing Removal of standard http Port")
  {
    REQUIRE(normalize_uri_helper("http://foobar.com:80/Something/Here", "http://foobar.com/Something/Here"));
  }

  SECTION("Testing Removal of standard https Port")
  {
    REQUIRE(normalize_uri_helper("https://foobar.com:443/Something/Here", "https://foobar.com/Something/Here"));
  }

  SECTION("Testing passing of non-standard http Port")
  {
    REQUIRE(normalize_uri_helper("http://foobar.com:443/Something/Here", "http://foobar.com:443/Something/Here"));
  }

  SECTION("Testing passing of non-standard https Port")
  {
    REQUIRE(normalize_uri_helper("https://foobar.com:80/Something/Here", "https://foobar.com:80/Something/Here"));
  }

  SECTION("Testing the removal of . and .. in the path ")
  {
    REQUIRE(
      normalize_uri_helper("https://foobar.com:80/Something/Here/././foobar/../foo", "https://foobar.com:80/Something/Here/foo"));
  }

  SECTION("Testing . and .. segments in non path components")
  {
    REQUIRE(normalize_uri_helper("https://foobar.com:80/Something/Here?query1=/././foo/../bar",
                                 "https://foobar.com:80/Something/Here?query1=/././foo/../bar"));
  }

  SECTION("Testing standard decdoing of multiple characters")
  {
    REQUIRE(normalize_uri_helper("https://kelloggs%54ester.com/%53omething/Here", "https://kelloggstester.com/Something/Here"));
  }

  SECTION("Testing passing encoded reserved characters")
  {
    REQUIRE(
      normalize_uri_helper("https://kelloggs%54ester.com/%53omething/Here%3f", "https://kelloggstester.com/Something/Here%3F"));
  }

  SECTION("Mixed Bag Test case")
  {
    REQUIRE(normalize_uri_helper("https://foo:something@kellogs%54ester.com:443/%53omething/.././here",
                                 "https://foo:something@kellogstester.com/here"));
  }

  SECTION("Testing empty hostname with userinfon")
  {
    REQUIRE(!normalize_uri_helper("https://foo:something@", NULL));
  }

  SECTION("Testing empty uri after http://")
  {
    REQUIRE(!normalize_uri_helper("http://", NULL));
  }

  SECTION("Testing http:///////")
  {
    REQUIRE(!normalize_uri_helper("http:///////", NULL));
  }

  SECTION("Testing empty uri after http://?/")
  {
    REQUIRE(!normalize_uri_helper("http://?/", NULL));
  }
  fprintf(stderr, "\n");
}

TEST_CASE("5", "[RegexTests]")
{
  INFO("TEST 5, Test Regex Matching");

  SECTION("Standard regex")
  {
    REQUIRE(match_regex("http://kelloggsTester.souza.local/KellogsDir/*",
                        "http://kelloggsTester.souza.local/KellogsDir/some_manifest.m3u8"));
  }

  SECTION("Back references are not supported")
  {
    REQUIRE(!match_regex("(b*a)\\1$", "bbbbba"));
  }

  SECTION("Escape a special character")
  {
    REQUIRE(match_regex("money\\$", "money$bags"));
  }

  SECTION("Dollar sign")
  {
    REQUIRE(!match_regex(".+foobar$", "foobarfoofoo"));
    REQUIRE(match_regex(".+foobar$", "foofoofoobar"));
  }

  SECTION("Number Quantifier with Groups")
  {
    REQUIRE(match_regex("(abab){2}", "abababab"));
    REQUIRE(!match_regex("(abab){2}", "abab"));
  }

  SECTION("Alternation")
  {
    REQUIRE(match_regex("cat|dog", "dog"));
  }
  fprintf(stderr, "\n");
}

TEST_CASE("6", "[AudTests]")
{
  INFO("TEST 6, Test Aud Matching");

  json_error_t *err = NULL;
  SECTION("Standard aud string match")
  {
    json_t *raw = json_loads("{\"aud\": \"tester\"}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  SECTION("Standard aud array match")
  {
    json_t *raw = json_loads("{\"aud\": [ \"foo\", \"bar\",  \"tester\"]}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  SECTION("Standard aud string mismatch")
  {
    json_t *raw = json_loads("{\"aud\": \"foo\"}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(!jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  SECTION("Standard aud array mismatch")
  {
    json_t *raw = json_loads("{\"aud\": [\"foo\", \"bar\", \"foobar\"]}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(!jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  SECTION("Integer trying to pass as an aud")
  {
    json_t *raw = json_loads("{\"aud\": 1}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(!jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  SECTION("Integer mixed into a passing aud array")
  {
    json_t *raw = json_loads("{\"aud\": [1, \"foo\", \"bar\", \"tester\"]}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  SECTION("Case sensitive test for single string")
  {
    json_t *raw = json_loads("{\"aud\": \"TESTer\"}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(!jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  SECTION("Case sensitive test for array")
  {
    json_t *raw = json_loads("{\"aud\": [1, \"foo\", \"bar\", \"Tester\"]}", 0, err);
    json_t *aud = json_object_get(raw, "aud");
    REQUIRE(!jwt_check_aud(aud, "tester"));
    json_decref(raw);
  }

  fprintf(stderr, "\n");
}

TEST_CASE("7", "[TestsConfig]")
{
  INFO("TEST 7, Config Loading and Config Functions");

  fprintf(stderr, "%s\n", testConfig);
  fflush(stderr);

  SECTION("Config Loading ID Field")
  {
    struct config *cfg = read_config_from_string(testConfig);
    REQUIRE(cfg != NULL);
    REQUIRE(strcmp(config_get_id(cfg), "tester") == 0);
    config_delete(cfg);
  }
  fprintf(stderr, "\n");
}

bool
jws_validation_helper(const char *url, const char *package, struct config *cfg)
{
  size_t url_ct   = strlen(url);
  size_t strip_ct = 0;
  char uri_strip[url_ct + 1];
  memset(uri_strip, 0, sizeof uri_strip);
  cjose_jws_t *jws = get_jws_from_uri(url, url_ct, package, uri_strip, url_ct, &strip_ct);
  if (!jws) {
    return false;
  }
  struct jwt *jwt = validate_jws(jws, cfg, uri_strip, strip_ct);
  cjose_jws_release(jws);
  if (!jwt) {
    return false;
  }
  jwt_delete(jwt);
  return true;
}

TEST_CASE("8", "[TestsWithConfig]")
{
  INFO("TEST 8, Tests Involving Validation with Config");
  struct config *cfg = read_config_from_string(testConfig);

  SECTION("Validation of Valid Aud String in JWS")
  {
    REQUIRE(jws_validation_helper("http://www.foobar.com/"
                                  "URISigningPackage=eyJLZXlJREtleSI6IjUiLCJhbGciOiJIUzI1NiJ9."
                                  "eyJjZG5pZXRzIjozMCwiY2RuaXN0dCI6MSwiaXNzIjoiTWFzdGVyIElzc3VlciIsImF1ZCI6InRlc3RlciIsImNkbml1YyI6"
                                  "InJlZ2V4Omh0dHA6Ly93d3cuZm9vYmFyLmNvbS8qIn0.InBxVm6OOAglNqc-U5wAZaRQVebJ9PK7Y9i7VFHWYHU",
                                  "URISigningPackage", cfg));
    fprintf(stderr, "\n");
  }

  SECTION("Validation of Invalid Aud String in JWS")
  {
    REQUIRE(!jws_validation_helper("http://www.foobar.com/"
                                   "URISigningPackage=eyJLZXlJREtleSI6IjUiLCJhbGciOiJIUzI1NiJ9."
                                   "eyJjZG5pZXRzIjozMCwiY2RuaXN0dCI6MSwiaXNzIjoiTWFzdGVyIElzc3VlciIsImF1ZCI6ImJhZCIsImNkbml1YyI6InJ"
                                   "lZ2V4Omh0dHA6Ly93d3cuZm9vYmFyLmNvbS8qIn0.aCOo8gOBa5G1RKkkzgWYwc79dPRw_fQUC0k1sWcjkyM",
                                   "URISigningPackage", cfg));
    fprintf(stderr, "\n");
  }

  SECTION("Validation of Valid Aud Array in JWS")
  {
    REQUIRE(jws_validation_helper(
      "http://www.foobar.com/"
      "URISigningPackage=eyJLZXlJREtleSI6IjUiLCJhbGciOiJIUzI1NiJ9."
      "eyJjZG5pZXRzIjozMCwiY2RuaXN0dCI6MSwiaXNzIjoiTWFzdGVyIElzc3VlciIsImF1ZCI6WyJiYWQiLCJpbnZhbGlkIiwidGVzdGVyIl0sImNkbml1YyI6InJl"
      "Z2V4Omh0dHA6Ly93d3cuZm9vYmFyLmNvbS8qIn0.7lyepZMzc_odieKvOTN2U-k1gLwRKS8KJIvDFQXDqGs",
      "URISigningPackage", cfg));
    fprintf(stderr, "\n");
  }

  SECTION("Validation of Invalid Aud Array in JWS")
  {
    REQUIRE(!jws_validation_helper(
      "http://www.foobar.com/"
      "URISigningPackage=eyJLZXlJREtleSI6IjUiLCJhbGciOiJIUzI1NiJ9."
      "eyJjZG5pZXRzIjozMCwiY2RuaXN0dCI6MSwiaXNzIjoiTWFzdGVyIElzc3VlciIsImF1ZCI6WyJiYWQiLCJpbnZhbGlkIiwiZm9vYmFyIl0sImNkbml1YyI6InJl"
      "Z2V4Omh0dHA6Ly93d3cuZm9vYmFyLmNvbS8qIn0.CU3WMJAPs0uRC7NKXvatVG9uU9SANdZzqO0GdQUatxk",
      "URISigningPackage", cfg));
    fprintf(stderr, "\n");
  }

  SECTION("Validation of Valid Aud Array Mixed types in JWS")
  {
    REQUIRE(jws_validation_helper(
      "http://www.foobar.com/"
      "URISigningPackage=eyJLZXlJREtleSI6IjUiLCJhbGciOiJIUzI1NiJ9."
      "eyJjZG5pZXRzIjozMCwiY2RuaXN0dCI6MSwiaXNzIjoiTWFzdGVyIElzc3VlciIsImF1ZCI6WyJiYWQiLDEsImZvb2JhciIsInRlc3RlciJdLCJjZG5pdWMiOiJy"
      "ZWdleDpodHRwOi8vd3d3LmZvb2Jhci5jb20vKiJ9._vlXsA3r7RPje2ZdMnpaGTwIsdNMjuQWPEHRkGKTVL8",
      "URISigningPackage", cfg));
    fprintf(stderr, "\n");
  }

  config_delete(cfg);
  fprintf(stderr, "\n");
}
