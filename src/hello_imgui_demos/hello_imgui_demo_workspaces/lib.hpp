#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>

#include "provider.hpp"

namespace workspace_lib {
  using ProviderFactoryFunc = std::function<std::shared_ptr<Provider>()>;

  void RegisterProviderType(const std::string& providerType, ProviderFactoryFunc &&factoryFunc);

  template<typename ProviderType>
  void RegisterProviderType(const std::string& providerType)
  {
    RegisterProviderType(providerType, []() { return std::make_shared<ProviderType>(); });
  }

  std::vector<std::string> GetProviderTypes();
  std::shared_ptr<Provider> CreateProvider(const std::string& providerType);

  void RemoveProviders();

  void OpenPopup(const std::string& popupName);
  std::vector<std::string>& GetOpenPopups();
}