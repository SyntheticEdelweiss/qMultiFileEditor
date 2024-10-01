#ifndef UTILS_H
#define UTILS_H

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QRegularExpression>

#include <functional>
#include <type_traits>

#ifdef under_cast
#error "under_cast macro already defined somewhere before - resolve this manually"
#else
#define under_cast(enumValue) \
    static_cast<std::underlying_type<decltype(enumValue)>::type>(enumValue)
#endif

#define g_presetsPath "../etc/qMultiFileEditor_Presets.ini"
#define g_settingsPath "../etc/qMultiFileEditor_Settings.ini"

QString regExpFromWildcardFilters(const QString& inputString);

enum class ActionType : int
{
    Remove,
    Replace
};

enum class ActionTarget : std::underlying_type_t<QDir::Filter>
{
    Files = QDir::Files,
    Dirs = QDir::Dirs,
    FilesDirs = QDir::Files | QDir::Dirs,
    FileContents
};
constexpr inline ActionTarget operator&(const ActionTarget& lhs, const ActionTarget& rhs)
{
    std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<ActionTarget>>(lhs)
                                               & static_cast<std::underlying_type_t<ActionTarget>>(rhs);
    return static_cast<ActionTarget>(ret_t);
}
template<typename T>
constexpr inline ActionTarget operator&(const ActionTarget& lhs, const T& rhs)
{
    if constexpr(std::is_enum<T>::value == true)
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, std::underlying_type_t<T>>::value == true,
            "operator&(ActionTarget, T) cannot be used with enum T of different underlying type");
        std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<ActionTarget>>(lhs)
                                                   & static_cast<std::underlying_type_t<T>>(rhs);
        return static_cast<ActionTarget>(ret_t);
    }
    else
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, T>::value == true,
            "operator&(ActionTarget, T) cannot be used with type T which is different from ActionTarget underlying type");
        std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<ActionTarget>>(lhs)
                                                   & rhs;
        return static_cast<ActionTarget>(ret_t);
    }
}
template<typename T>
constexpr inline ActionTarget operator&(const T& lhs, const ActionTarget& rhs)
{
    if constexpr(std::is_enum<T>::value == true)
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, std::underlying_type_t<T>>::value == true,
            "operator&(T, ActionTarget) cannot be used with enum T of different underlying type");
        std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<T>>(lhs)
                                                   & static_cast<std::underlying_type_t<ActionTarget>>(rhs);
        return static_cast<ActionTarget>(ret_t);
    }
    else
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, T>::value == true,
            "operator&(T, ActionTarget) cannot be used with type T which is different from ActionTarget underlying type");
        std::underlying_type_t<ActionTarget> ret_t = lhs
                                                   & static_cast<std::underlying_type_t<ActionTarget>>(rhs);
        return static_cast<ActionTarget>(ret_t);
    }
}
constexpr inline ActionTarget operator|(const ActionTarget& lhs, const ActionTarget& rhs)
{
    std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<ActionTarget>>(lhs)
                                               | static_cast<std::underlying_type_t<ActionTarget>>(rhs);
    return static_cast<ActionTarget>(ret_t);
}
template<typename T>
constexpr inline ActionTarget operator|(const ActionTarget& lhs, const T& rhs)
{
    if constexpr(std::is_enum<T>::value == true)
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, std::underlying_type_t<T>>::value == true,
            "operator&(ActionTarget, T) cannot be used with enum T of different underlying type");
        std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<ActionTarget>>(lhs)
                                                   | static_cast<std::underlying_type_t<T>>(rhs);
        return static_cast<ActionTarget>(ret_t);
    }
    else
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, T>::value == true,
            "operator&(ActionTarget, T) cannot be used with type T which is different from ActionTarget underlying type");
        std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<ActionTarget>>(lhs)
                                                   | rhs;
        return static_cast<ActionTarget>(ret_t);
    }
}
template<typename T>
constexpr inline ActionTarget operator|(const T& lhs, const ActionTarget& rhs)
{
    if constexpr(std::is_enum<T>::value == true)
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, std::underlying_type_t<T>>::value == true,
            "operator&(T, ActionTarget) cannot be used with enum T of different underlying type");
        std::underlying_type_t<ActionTarget> ret_t = static_cast<std::underlying_type_t<T>>(lhs)
                                                   | static_cast<std::underlying_type_t<ActionTarget>>(rhs);
        return static_cast<ActionTarget>(ret_t);
    }
    else
    {
        static_assert(std::is_same<std::underlying_type_t<ActionTarget>, T>::value == true,
            "operator&(T, ActionTarget) cannot be used with type T which is different from ActionTarget underlying type");
        std::underlying_type_t<ActionTarget> ret_t = lhs
                                                   | static_cast<std::underlying_type_t<ActionTarget>>(rhs);
        return static_cast<ActionTarget>(ret_t);
    }
}

struct MFEPreset
{
    int actionType;
    int actionTarget;
    bool isRecursive;
    bool isCaseSensitive;
    bool isAutoconfirmExecute;
    bool isRegExpFilePattern;
    bool isRegExpSearchReplace;
    bool isHighlightMatch;
    QString dirPath;
    QString filePattern;
    QString searchFor;
    QString replaceWith;
    QString presetName;
};

struct FileInfoArgs
{
    QStringList nameFilters;
    QDir::Filters filters;
    QDir::SortFlags sortFlags;
};

/* Meant to temporarily disable some widgets while executing some function and to enable them back when that function is finished.
 * Calls f_enableWidgets(false) in constructor to disable specified widgets.
 * Calls f_enableWidgets(true) in destructor to enable previously disabled widgets. */
class WidgetsDisabler
{
public:
    ~WidgetsDisabler()
    {
        f_enableWidgets(true);
    }
    WidgetsDisabler(std::function<void(bool)> f_enablerFunc) : f_enableWidgets(f_enablerFunc)
    {
        f_enableWidgets(false);
    }
    WidgetsDisabler(const WidgetsDisabler&) = delete;
    WidgetsDisabler(WidgetsDisabler&&) = delete;
    WidgetsDisabler& operator=(const WidgetsDisabler&) = delete;
    WidgetsDisabler& operator=(WidgetsDisabler&&) = delete;

private:
    std::function<void(bool)> f_enableWidgets;
};

static_assert(std::is_same<std::underlying_type<ActionTarget>::type, int>::value == true,
    "underlying type of ActionTarget has to be compatible with QVariant::toInt() to store it in QComboBox::userData");
static_assert(std::is_same<std::underlying_type<ActionType>::type, int>::value == true,
    "underlying type of ActionType has to be compatible with QVariant::toInt() to store it in QComboBox::userData");

#endif // UTILS_H
