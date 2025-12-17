#include <Arduino.h>

class ConfigManagerError
{
    private:
        int _errorCode = 0;
        String _errorString;
    public:
        ConfigManagerError(
            int errorCode,
            String errorString
        );
        bool isError();
        int getErrorCode();
        String getErrorString();
}; 

ConfigManagerError::ConfigManagerError(
    int errorCode,
    String errorString
) {
    _errorCode = errorCode;
    _errorString = errorString;
}

bool ConfigManagerError::isError() {
    return _errorCode == 0;
}

int ConfigManagerError::getErrorCode() {
    return _errorCode;
}

String ConfigManagerError::getErrorString() {
    return _errorString;
}
