#include <gtest/gtest.h>

#include <alpaca/markets/rest/config.hpp>

using namespace alpaca::markets;

TEST(RetryConfigTest, DefaultConfig) {
    RetryConfig config;
    
    EXPECT_EQ(config.max_retries, 3);
    EXPECT_EQ(config.initial_delay.count(), 100);
    EXPECT_EQ(config.max_delay.count(), 5000);
    EXPECT_DOUBLE_EQ(config.backoff_multiplier, 2.0);
}

TEST(RetryConfigTest, ShouldRetry) {
    RetryConfig config;
    
    // Should retry on rate limit and server errors
    EXPECT_TRUE(config.shouldRetry(429));
    EXPECT_TRUE(config.shouldRetry(500));
    EXPECT_TRUE(config.shouldRetry(502));
    EXPECT_TRUE(config.shouldRetry(503));
    EXPECT_TRUE(config.shouldRetry(504));
    
    // Should not retry on client errors
    EXPECT_FALSE(config.shouldRetry(400));
    EXPECT_FALSE(config.shouldRetry(401));
    EXPECT_FALSE(config.shouldRetry(403));
    EXPECT_FALSE(config.shouldRetry(404));
    EXPECT_FALSE(config.shouldRetry(422));
    
    // Should not retry on success
    EXPECT_FALSE(config.shouldRetry(200));
    EXPECT_FALSE(config.shouldRetry(201));
}

TEST(RetryConfigTest, GetDelay) {
    RetryConfig config;
    config.initial_delay = std::chrono::milliseconds{100};
    config.backoff_multiplier = 2.0;
    config.max_delay = std::chrono::milliseconds{5000};
    
    EXPECT_EQ(config.getDelay(0).count(), 100);
    EXPECT_EQ(config.getDelay(1).count(), 200);
    EXPECT_EQ(config.getDelay(2).count(), 400);
    EXPECT_EQ(config.getDelay(3).count(), 800);
}

TEST(RetryConfigTest, GetDelayMaxCapped) {
    RetryConfig config;
    config.initial_delay = std::chrono::milliseconds{1000};
    config.backoff_multiplier = 10.0;
    config.max_delay = std::chrono::milliseconds{5000};
    
    // 1000 * 10 * 10 = 100000, but capped at 5000
    EXPECT_EQ(config.getDelay(2).count(), 5000);
}

TEST(RetryConfigTest, NoRetries) {
    RetryConfig config = RetryConfig::noRetries();
    
    EXPECT_EQ(config.max_retries, 0);
}

TEST(TimeoutConfigTest, DefaultConfig) {
    TimeoutConfig config;
    
    EXPECT_EQ(config.connection_timeout.count(), 10);
    EXPECT_EQ(config.read_timeout.count(), 30);
    EXPECT_EQ(config.write_timeout.count(), 30);
}

TEST(TimeoutConfigTest, LongTimeouts) {
    TimeoutConfig config = TimeoutConfig::longTimeouts();
    
    EXPECT_EQ(config.connection_timeout.count(), 30);
    EXPECT_EQ(config.read_timeout.count(), 60);
    EXPECT_EQ(config.write_timeout.count(), 60);
}

TEST(EnvironmentConfigTest, RetryConfig) {
    Environment env;
    
    // Default retry config
    EXPECT_EQ(env.getRetryConfig().max_retries, 3);
    
    // Set custom retry config
    RetryConfig custom;
    custom.max_retries = 5;
    env.setRetryConfig(custom);
    
    EXPECT_EQ(env.getRetryConfig().max_retries, 5);
}

TEST(EnvironmentConfigTest, TimeoutConfig) {
    Environment env;
    
    // Default timeout config
    EXPECT_EQ(env.getTimeoutConfig().connection_timeout.count(), 10);
    
    // Set custom timeout config
    TimeoutConfig custom = TimeoutConfig::longTimeouts();
    env.setTimeoutConfig(custom);
    
    EXPECT_EQ(env.getTimeoutConfig().connection_timeout.count(), 30);
}
