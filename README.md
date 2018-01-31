# ZWUtils-CPP
[![License](https://img.shields.io/github/license/Adam5Wu/ZWUtils-Java.svg)](./LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/Adam5Wu/ZWUtils-Java.svg)](https://github.com/Adam5Wu/ZWUtils-Java/issues)
[![GitHub forks](https://img.shields.io/github/forks/Adam5Wu/ZWUtils-Java.svg)](https://github.com/Adam5Wu/ZWUtils-Java/network)
[![Build Status](https://travis-ci.org/Adam5Wu/ZWUtils-Java.svg?branch=master)](https://travis-ci.org/Adam5Wu/ZWUtils-Java)
[![SonarCloud-Stat](https://sonarcloud.io/api/badges/gate?key=ZWUtils-Java)](https://sonarcloud.io/dashboard?id=ZWUtils-Java)

[![SonarCloud-SLoC](https://sonarcloud.io/api/badges/measure?key=ZWUtils-Java&metric=lines)](https://sonarcloud.io/dashboard?id=ZWUtils-Java)
[![SonarCloud-Bugs](https://sonarcloud.io/api/badges/measure?key=ZWUtils-Java&metric=bugs)](https://sonarcloud.io/dashboard?id=ZWUtils-Java)
[![SonarCloud-Vuls](https://sonarcloud.io/api/badges/measure?key=ZWUtils-Java&metric=vulnerabilities)](https://sonarcloud.io/dashboard?id=ZWUtils-Java)
[![SonarCloud-Smls](https://sonarcloud.io/api/badges/measure?key=ZWUtils-Java&metric=code_smells)](https://sonarcloud.io/dashboard?id=ZWUtils-Java)
[![SonarCloud-Cvge](https://sonarcloud.io/api/badges/measure?key=ZWUtils-Java&metric=coverage)](https://sonarcloud.io/dashboard?id=ZWUtils-Java)

This utility library provides basic run-time services, such as log management, configuration loading and saving, thread synchronization primitive and objects, memory debugging support, etc. to C++ applications.

It is the rewrite of [ZWUtils-VCPP](https://github.com/Adam5Wu/ZWUtils-VCPP), which was mainly targeting Visual C++. Instead, this library aims at cross-platform compatibility. In addition, it fully embraces C++ RAII, which brings BIG performance improvement compared with using __finally clause, something I discovered after the predecessor library is mostly completed. I also try to employ C++11 features where they could improve performance/code readability.

# Acknowledgement
I am grateful for NEC Laboratories America, Inc., for granting open source release of this library.

# License
BSD 3-clause New License

# How to Use
(Coming soon in wiki)
