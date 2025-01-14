Name:           ohao-ide
Version:        1.0.4
Release:        1%{?dist}
Summary:        Ohao IDE - A lightweight IDE
License:        MIT
URL:            https://github.com/yourusername/ohao-ide
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtpdf-devel
BuildRequires:  cups-devel
BuildRequires:  make
BuildRequires:  desktop-file-utils

Requires:       qt6-qtbase
Requires:       qt6-qtpdf
Requires:       qt6-qtbase-gui
Requires:       cups-libs

%description
Ohao IDE is a lightweight integrated development environment with PDF preview support.

%prep
%autosetup

%build
# Create necessary files if they don't exist
echo "# Ohao IDE" > README.md
mkdir -p resources/styles
touch resources/styles/dark.qss

# Create desktop entry file
cat > %{name}.desktop << EOF
[Desktop Entry]
Name=Ohao IDE
Comment=Modern C++ IDE
Exec=ohao_IDE
Icon=text-editor
Terminal=false
Type=Application
Categories=Development;IDE;Qt;
Keywords=development;programming;IDE;
EOF

%cmake
%cmake_build

%install
%cmake_install

# Install desktop file only
mkdir -p %{buildroot}%{_datadir}/applications
desktop-file-install --dir=%{buildroot}%{_datadir}/applications %{name}.desktop

%files
%{_bindir}/ohao_IDE
%{_datadir}/doc/ohao_IDE/LICENSE
%{_datadir}/doc/ohao_IDE/README.md
%{_datadir}/applications/%{name}.desktop
%{_datadir}/ohao_IDE/resources.qrc
%license LICENSE

%changelog
* Wed Jan 08 2025 Qervas <djmax96945147@outlook.com> - 1.0.0-1
- Initial RPM release, support dark mode
- Support find and replace
