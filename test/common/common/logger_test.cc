#include <memory>
#include <string>

#include "source/common/common/json_escape_string.h"
#include "source/common/common/logger.h"

#include "test/test_common/environment.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace Envoy {
namespace Logger {

TEST(LoggerTest, StackingStderrSinkDelegate) {
  StderrSinkDelegate stacked(Envoy::Logger::Registry::getSink());
}

TEST(LoggerEscapeTest, LinuxEOL) {
  EXPECT_EQ("line 1 \\n line 2\n", DelegatingLogSink::escapeLogLine("line 1 \n line 2\n"));
}

TEST(LoggerEscapeTest, WindowEOL) {
  EXPECT_EQ("line 1 \\n line 2\r\n", DelegatingLogSink::escapeLogLine("line 1 \n line 2\r\n"));
}

TEST(LoggerEscapeTest, NoTrailingWhitespace) {
  EXPECT_EQ("line 1 \\n line 2", DelegatingLogSink::escapeLogLine("line 1 \n line 2"));
}

TEST(LoggerEscapeTest, NoWhitespace) {
  EXPECT_EQ("line1", DelegatingLogSink::escapeLogLine("line1"));
}

TEST(LoggerEscapeTest, AnyTrailingWhitespace) {
  EXPECT_EQ("line 1 \\t tab 1 \\n line 2\t\n",
            DelegatingLogSink::escapeLogLine("line 1 \t tab 1 \n line 2\t\n"));
}

TEST(LoggerEscapeTest, WhitespaceOnly) {
  // 8 spaces
  EXPECT_EQ("        ", DelegatingLogSink::escapeLogLine("        "));

  // Any whitespace characters
  EXPECT_EQ("\r\n\t \r\n \n", DelegatingLogSink::escapeLogLine("\r\n\t \r\n \n"));
}

TEST(LoggerEscapeTest, Empty) { EXPECT_EQ("", DelegatingLogSink::escapeLogLine("")); }

TEST(JsonEscapeTest, Escape) {
  const auto expect_json_escape = [](absl::string_view to_be_escaped, absl::string_view escaped) {
    EXPECT_EQ(escaped,
              JsonEscaper::escapeString(to_be_escaped, JsonEscaper::extraSpace(to_be_escaped)));
  };

  expect_json_escape("\"", "\\\"");
  expect_json_escape("\\", "\\\\");
  expect_json_escape("\b", "\\b");
  expect_json_escape("\f", "\\f");
  expect_json_escape("\n", "\\n");
  expect_json_escape("\r", "\\r");
  expect_json_escape("\t", "\\t");
  expect_json_escape("\x01", "\\u0001");
  expect_json_escape("\x02", "\\u0002");
  expect_json_escape("\x03", "\\u0003");
  expect_json_escape("\x04", "\\u0004");
  expect_json_escape("\x05", "\\u0005");
  expect_json_escape("\x06", "\\u0006");
  expect_json_escape("\x07", "\\u0007");
  expect_json_escape("\x08", "\\b");
  expect_json_escape("\x09", "\\t");
  expect_json_escape("\x0a", "\\n");
  expect_json_escape("\x0b", "\\u000b");
  expect_json_escape("\x0c", "\\f");
  expect_json_escape("\x0d", "\\r");
  expect_json_escape("\x0e", "\\u000e");
  expect_json_escape("\x0f", "\\u000f");
  expect_json_escape("\x10", "\\u0010");
  expect_json_escape("\x11", "\\u0011");
  expect_json_escape("\x12", "\\u0012");
  expect_json_escape("\x13", "\\u0013");
  expect_json_escape("\x14", "\\u0014");
  expect_json_escape("\x15", "\\u0015");
  expect_json_escape("\x16", "\\u0016");
  expect_json_escape("\x17", "\\u0017");
  expect_json_escape("\x18", "\\u0018");
  expect_json_escape("\x19", "\\u0019");
  expect_json_escape("\x1a", "\\u001a");
  expect_json_escape("\x1b", "\\u001b");
  expect_json_escape("\x1c", "\\u001c");
  expect_json_escape("\x1d", "\\u001d");
  expect_json_escape("\x1e", "\\u001e");
  expect_json_escape("\x1f", "\\u001f");
}

class LoggerCustomFlagsTest : public testing::Test {
public:
  LoggerCustomFlagsTest() : logger_(Registry::getSink()) {}

  void expectLogMessage(const std::string& pattern, const std::string& message,
                        const std::string& expected) {
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter
        ->add_flag<CustomFlagFormatter::EscapeMessageNewLine>(
            CustomFlagFormatter::EscapeMessageNewLine::Placeholder)
        .set_pattern(pattern);
    formatter
        ->add_flag<CustomFlagFormatter::EscapeMessageJsonString>(
            CustomFlagFormatter::EscapeMessageJsonString::Placeholder)
        .set_pattern(pattern);
    logger_->set_formatter(std::move(formatter));

    testing::internal::CaptureStderr();
    logger_->log(spdlog::details::log_msg("test", spdlog::level::info, message));
    EXPECT_EQ(absl::StrCat(expected, TestEnvironment::newLine),
              testing::internal::GetCapturedStderr());
  }

protected:
  DelegatingLogSinkSharedPtr logger_;
};

TEST_F(LoggerCustomFlagsTest, LogMessageAsIs) {
  // This uses "%v", the default flag for printing the actual text to log.
  // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags.
  expectLogMessage("%v", "\n\nmessage\n\n", "\n\nmessage\n\n");
}

TEST_F(LoggerCustomFlagsTest, LogMessageAsEscaped) {
  // This uses "%_", the added custom flag that escapes newlines from the actual text to log.
  expectLogMessage("%_", "\n\nmessage\n\n", "\\n\\nmessage\\n\\n");
}

TEST_F(LoggerCustomFlagsTest, LogMessageAsJsonStringEscaped) {
  // This uses "%j", the added custom flag that JSON escape the characters inside the log message
  // payload.
  expectLogMessage("%j", "message", "message");
  expectLogMessage("%j", "\n\nmessage\n\n", "\\n\\nmessage\\n\\n");
  expectLogMessage("%j", "\bok\b", "\\bok\\b");
  expectLogMessage("%j", "\fok\f", "\\fok\\f");
  expectLogMessage("%j", "\rok\r", "\\rok\\r");
  expectLogMessage("%j", "\tok\t", "\\tok\\t");
  expectLogMessage("%j", "\\ok\\", "\\\\ok\\\\");
  expectLogMessage("%j", "\"ok\"", "\\\"ok\\\"");
  expectLogMessage("%j", "\x01ok\x0e", "\\u0001ok\\u000e");
  expectLogMessage(
      "%j",
      "StreamAggregatedResources gRPC config stream closed: 14, connection error: desc = "
      "\"transport: Error while dialing dial tcp [::1]:15012: connect: connection refused\"",
      "StreamAggregatedResources gRPC config stream closed: 14, connection error: desc = "
      "\\\"transport: Error while dialing dial tcp [::1]:15012: connect: connection refused\\\"");
}

struct NamedLogSink : SinkDelegate {
  NamedLogSink(DelegatingLogSinkSharedPtr log_sink) : SinkDelegate(log_sink) { setDelegate(); }
  ~NamedLogSink() override { restoreDelegate(); }

  MOCK_METHOD(void, log, (absl::string_view));
  MOCK_METHOD(void, logWithStableName,
              (absl::string_view, absl::string_view, absl::string_view, absl::string_view));
  void flush() override {}
};

class NamedLogTest : public Loggable<Id::assert>, public testing::Test {};

TEST_F(NamedLogTest, NamedLogsAreSentToSink) {
  NamedLogSink sink(Envoy::Logger::Registry::getSink());

  Envoy::Logger::Registry::setLogLevel(spdlog::level::info);
  // Log level is above debug, so we shouldn't get any logs.
  ENVOY_LOG_EVENT(debug, "test_event", "not logged");

  Envoy::Logger::Registry::setLogLevel(spdlog::level::debug);

  EXPECT_CALL(sink, log(_));
  EXPECT_CALL(sink, logWithStableName("test_event", "debug", "assert", "test log 1"));
  ENVOY_LOG_EVENT(debug, "test_event", "test {} {}", "log", 1);

  // Verify that ENVOY_LOG_EVENT_TO_LOGGER does the right thing.
  EXPECT_CALL(sink, log(_)).WillOnce(Invoke([](auto log) { EXPECT_TRUE(log.find("[misc]")); }));
  EXPECT_CALL(sink, logWithStableName("misc_event", "debug", "misc", "log"));
  ENVOY_LOG_EVENT_TO_LOGGER(Registry::getLog(Id::misc), debug, "misc_event", "log");
}

struct TlsLogSink : SinkDelegate {
  TlsLogSink(DelegatingLogSinkSharedPtr log_sink) : SinkDelegate(log_sink) { setTlsDelegate(); }
  ~TlsLogSink() override { restoreTlsDelegate(); }

  MOCK_METHOD(void, log, (absl::string_view));
  MOCK_METHOD(void, logWithStableName,
              (absl::string_view, absl::string_view, absl::string_view, absl::string_view));
  MOCK_METHOD(void, flush, ());
};

TEST(NamedLogtest, OverrideSink) {
  NamedLogSink global_sink(Envoy::Logger::Registry::getSink());
  testing::InSequence s;

  {
    TlsLogSink tls_sink(Envoy::Logger::Registry::getSink());

    // Calls on the current thread goes to the TLS sink.
    EXPECT_CALL(tls_sink, log(_));
    ENVOY_LOG_MISC(info, "hello tls");

    // Calls on other threads should use the global sink.
    std::thread([&]() {
      EXPECT_CALL(global_sink, log(_));
      ENVOY_LOG_MISC(info, "hello global");
    }).join();

    // Sanity checking that we're still using the TLS sink.
    EXPECT_CALL(tls_sink, log(_));
    ENVOY_LOG_MISC(info, "hello tls");

    // All the logging functions should be delegated to the TLS override.
    EXPECT_CALL(tls_sink, flush());
    Registry::getSink()->flush();

    EXPECT_CALL(tls_sink, logWithStableName(_, _, _, _));
    Registry::getSink()->logWithStableName("foo", "level", "bar", "msg");
  }

  // Now that the TLS sink is out of scope, log calls on this thread should use the global sink
  // again.
  EXPECT_CALL(global_sink, log(_));
  ENVOY_LOG_MISC(info, "hello global 2");
}
} // namespace Logger
} // namespace Envoy
