#include "lib.hpp"
#include "provider.hpp"

#include <memory>
#include <unordered_map>

namespace workspace_lib
{

namespace impl
{
    static std::vector<std::shared_ptr<Provider>> s_providers;
    static std::unordered_map<std::string, ProviderFactoryFunc> s_providerFactories;
};  // namespace impl

void RegisterProviderType(const std::string& providerType, ProviderFactoryFunc&& factoryFunc)
{
    impl::s_providerFactories[providerType] = factoryFunc;
}

std::vector<std::string> GetProviderTypes()
{
    std::vector<std::string> providerTypes;
    for (const auto& [providerType, _] : impl::s_providerFactories)
    {
        providerTypes.push_back(providerType);
    }
    return providerTypes;
}

std::shared_ptr<Provider> CreateProvider(const std::string& providerType)
{
    if (auto it = impl::s_providerFactories.find(providerType); it != impl::s_providerFactories.end())
    {
        auto provider = it->second();
        impl::s_providers.push_back(provider);
        return provider;
    }
    return nullptr;
}

void RemoveProviders() {
    // cleans up (calls destructor) for providers that GetShouldRemove() returns true
    impl::s_providers.erase(
        std::remove_if(
            impl::s_providers.begin(),
            impl::s_providers.end(),
            [](const std::shared_ptr<Provider>& provider) {
                return provider->GetShouldRemove();
            }),
        impl::s_providers.end());
}

static std::vector<std::string> s_openPopups;
void OpenPopup(const std::string& popupName)
{
    s_openPopups.push_back(popupName);
}

std::vector<std::string>& GetOpenPopups()
{
    return s_openPopups;
}

}  // namespace workspace_lib
