#pragma once


// Forward



class AFMakeDirectory final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFMakeDirectory() = default;
	~AFMakeDirectory() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
#pragma endregion public func
    bool                    MakeUserDirs();
    bool                    MakeUserProfileDirs();
#pragma region private func
private:
    bool                    _DoMkDir(const char* path);
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:

#pragma endregion private member var
};
