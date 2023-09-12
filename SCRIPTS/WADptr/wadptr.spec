
Name:		wadptr
Version:	2.4
Release:	0
Group:		Development/Tools/Building
Summary:	Redundancy compressor for Doom WAD files
License:	GPLv2+
URL:		http://soulsphere.org/projects/wadptr/

Source:		wadptr-%version.tar.gz
BuildRoot:      %_tmppath/%name-%version-build

%description
WADptr is a utility for reducing the size of Doom WAD files. The
"compressed" WADs will still work the same as the originals. The
program works by exploiting the WAD file format to combine repeated /
redundant material.

Authors:
--------
	Simon "Fraggle" Howard
	Andreas Dehmel

%prep
%setup

%build
make %{?_smp_mflags} linux;

%install
b="%buildroot";
make install DESTDIR="$b" PREFIX=/usr;
mkdir -p "$b/%_docdir/%name";
perl -i -pe 's/\x0d//gs' "$b/%_docdir/%name"/*.txt;

%files
%defattr(-,root,root)
%_bindir/*
%doc %_docdir/%name

%changelog
