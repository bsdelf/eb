#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class ArgumentParser {
 public:
  struct Result {
    std::map<std::string, std::string> args;
    std::vector<std::string> rests;
  };

  Result Parse(int argc, char** argv, const std::string& separator = "=") const {
    Result result{initial_key_values_, {}};
    for (int i = 0; i < argc; ++i) {
      std::string arg(argv[i]);
      if (arg.empty()) {
        continue;
      }
      std::string key;
      std::optional<std::string> value;
      if (auto pos = arg.find(separator); pos != std::string::npos) {
        key = arg.substr(0, pos);
        value = arg.substr(pos + separator.size());
      } else {
        key = arg;
      }
      auto iter = key_value_handlers_.find(key);
      if (iter == key_value_handlers_.end()) {
        result.rests.push_back(std::move(arg));
      } else {
        const auto& handlers = iter->second;
        for (size_t i = 0; i < handlers.size(); ++i) {
          handlers[i](result.args, value);
        }
      }
    }
    return result;
  }

  struct Set {
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::function<void(std::string&)> modifier;
    Set(std::optional<std::string> initial_value, std::optional<std::string> default_value)
      : initial_value(std::move(initial_value)), default_value(std::move(default_value)) {
    }
    Set(std::optional<std::string> initial_value, std::optional<std::string> default_value, std::function<void(std::string&)> modifier)
      : initial_value(std::move(initial_value)), default_value(std::move(default_value)), modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, const Set& set, const T&... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, set, rest...);
  }

  ArgumentParser& On(const std::string& key, const Set& set) {
    if (set.initial_value) {
      auto value = *set.initial_value;
      if (set.modifier) {
        set.modifier(value);
      }
      initial_key_values_.insert_or_assign(key, std::move(value));
    }
    auto handler =
      ([key = key,
        default_value = set.default_value,
        modifier = set.modifier](
         std::map<std::string, std::string>& data,
         std::optional<std::string> input_value) {
        auto& actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          data.insert_or_assign(key, std::move(value));
        }
      });
    return AddValueHandler(key, std::move(handler));
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, const Set& set, const T&... rest) {
    return On(key, set).On(key, rest...);
  }

  struct SetTo {
    std::string key;
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::function<void(std::string&)> modifier;
    SetTo(const std::string& key,
          std::optional<std::string> initial_value,
          std::optional<std::string> default_value)
      : key(key), initial_value(std::move(initial_value)), default_value(std::move(default_value)) {
    }
    SetTo(const std::string& key,
          std::optional<std::string> initial_value,
          std::optional<std::string> default_value,
          std::function<void(std::string&)> modifier)
      : key(key), initial_value(std::move(initial_value)), default_value(std::move(default_value)), modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, const SetTo& set_to, const T&... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, set_to, rest...);
  }

  ArgumentParser& On(const std::string& key, const SetTo& set_to) {
    if (set_to.initial_value) {
      auto value = *set_to.initial_value;
      if (set_to.modifier) {
        set_to.modifier(value);
      }
      initial_key_values_.insert_or_assign(set_to.key, std::move(value));
    }
    auto handler =
      ([to_key = set_to.key,
        default_value = set_to.default_value,
        modifier = set_to.modifier](
         std::map<std::string, std::string>& data,
         std::optional<std::string> input_value) {
        auto& actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          data.insert_or_assign(to_key, std::move(value));
        }
      });
    return AddValueHandler(key, std::move(handler));
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, const SetTo& set_to, const T&... rest) {
    return On(key, set_to).On(key, rest...);
  }

  struct Join {
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::string separator = " ";
    std::function<void(std::string&)> modifier;
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)) {
    }
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value,
         const std::string& separator)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator) {
    }
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value,
         std::function<void(std::string&)> modifier)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        modifier(std::move(modifier)) {
    }
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value,
         const std::string& separator,
         std::function<void(std::string&)> modifier)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator),
        modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, const Join& join, const T&... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, join, rest...);
  }

  ArgumentParser& On(const std::string& key, const Join& join) {
    if (join.initial_value) {
      auto value = *join.initial_value;
      if (join.modifier) {
        join.modifier(value);
      }
      auto iter = initial_key_values_.find(key);
      if (iter == initial_key_values_.end()) {
        initial_key_values_.emplace(key, std::move(value));
      } else if (iter->second.empty()) {
        iter->second = std::move(value);
      } else {
        iter->second += join.separator + value;
      }
    }
    auto handler =
      ([key = key,
        separator = join.separator,
        default_value = join.default_value,
        modifier = join.modifier](
         std::map<std::string, std::string>& data,
         std::optional<std::string> input_value) {
        auto& actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          auto iter = data.find(key);
          if (iter == data.end()) {
            data.emplace(key, std::move(value));
          } else if (iter->second.empty()) {
            iter->second = std::move(value);
          } else {
            iter->second += separator + value;
          }
        }
      });
    return AddValueHandler(key, std::move(handler));
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, const Join& join, const T&... rest) {
    return On(key, join).On(key, rest...);
  }

  struct JoinTo {
    std::string key;
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::string separator = " ";
    std::function<void(std::string&)> modifier;
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)) {
    }
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value,
           std::function<void(std::string&)> modifier)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        modifier(std::move(modifier)) {
    }
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value,
           const std::string& separator)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator) {
    }
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value,
           const std::string& separator,
           std::function<void(std::string&)> modifier)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator),
        modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, const JoinTo& join_to, const T&... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, join_to, rest...);
  }

  ArgumentParser& On(const std::string& key, const JoinTo& join_to) {
    if (join_to.initial_value) {
      auto value = *join_to.initial_value;
      if (join_to.modifier) {
        join_to.modifier(value);
      }
      auto iter = initial_key_values_.find(join_to.key);
      if (iter == initial_key_values_.end()) {
        initial_key_values_.emplace(join_to.key, std::move(value));
      } else if (iter->second.empty()) {
        iter->second = std::move(value);
      } else {
        iter->second += join_to.separator + value;
      }
    }
    auto handler =
      ([to_key = join_to.key,
        separator = join_to.separator,
        default_value = join_to.default_value,
        modifier = join_to.modifier](
         std::map<std::string, std::string>& data,
         std::optional<std::string> input_value) {
        auto& actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          auto iter = data.find(to_key);
          if (iter == data.end()) {
            data.emplace(to_key, std::move(value));
          } else if (iter->second.empty()) {
            iter->second = std::move(value);
          } else
            iter->second += separator + value;
        }
      });
    return AddValueHandler(key, std::move(handler));
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, const JoinTo& join_to, const T&... rest) {
    return On(key, join_to).On(key, rest...);
  }

  ArgumentParser& Split() {
    key_helps_.emplace_back();
    return *this;
  }

  void Clear() {
    key_helps_.clear();
    key_value_handlers_.clear();
    initial_key_values_.clear();
  }

  struct FormatHelpOptions {
    size_t space_before_key = 0;
    size_t min_space_before_value = 4;
    std::string newline = "\n";
  };

  std::string FormatHelp(const FormatHelpOptions& options) const {
    std::string result;
    size_t max = 0;
    for (const auto& item : key_helps_) {
      auto size = item.key.size();
      if (size > max) {
        max = size;
      }
    }
    const std::string space_before_key(options.space_before_key, ' ');
    for (const auto& item : key_helps_) {
      if (item.key.empty()) {
        result += options.newline;
        continue;
      }
      std::string space_before_value(max - item.key.size() + options.min_space_before_value, ' ');
      result += space_before_key + item.key + space_before_value + item.help + options.newline;
    }
    return result;
  }

 private:
  using ValueHandler = std::function<void(std::map<std::string, std::string>&, std::optional<std::string>)>;

  ArgumentParser& AddValueHandler(const std::string& key, ValueHandler&& handler) {
    auto iter = key_value_handlers_.find(key);
    if (iter == key_value_handlers_.end()) {
      key_value_handlers_.emplace(key, std::vector<ValueHandler>{std::move(handler)});
    } else {
      iter->second.push_back(std::move(handler));
    }
    return *this;
  }

 private:
  struct KeyHelp {
    std::string key;
    std::string help;
    KeyHelp() = default;
    KeyHelp(const std::string& key, const std::string& help)
      : key(key), help(help) {
    }
  };
  std::vector<KeyHelp> key_helps_;
  std::map<std::string, std::vector<ValueHandler>> key_value_handlers_;
  std::map<std::string, std::string> initial_key_values_;
};
