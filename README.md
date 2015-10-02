# STagS: Simple Tagging Service

Author: Wei Dong (wdong@wdong.org)

# Introduction

The goal of STagS is to experiment with the idea of a RESTful omnipotent
tagging service: upon receiving a POSTed chunk of data from the client, the
server will try to identify the type of data, and invoke various appropriate
backends to generate tagging data.  Recent years have witnessed a rapid
advancement in machine learning technologies as well as the emergence of many
high quality open-source software for processing visual, audio, textual and
other types of non-structural data.  STagS is to integrate many of such
software and provide a unified portal to them.

More specifically, STagS produces a single huge statically linked monolithic
Linux executable that is supposed to run on any Linux system with kernel
version >= 2.6 and with a X86_64 CPU.

## Disclaimer

This project is for fun and does not reflect my understanding of good software
engineering practice.  Readers/Users should understand that, static linking,
although bringing convenience in deployment, has obvious limitations, and is
considered by some people as a bad practice.

# Using the Service

A demo service is currently deployed, and can be invoked from commandline
with CURL:

```bash
curl -F "encoded_data=@text-sample.txt" -F "encoded_data=@image-sample.jpg" http://www.kgraph.org:3112/tag 
```

The API can also be accessed from a program, assuming multiple data files are
properly POSTed with multipart/form-data.


# Building and Running

It's a daunting task to build this project due to the numerous dependencies,
and running the program requires putting together pre-trained models.  At this
stage, this project is for my own fun.  You've been warned.

Email me if you want to replicate the service and contribute to the
development.

# Currently Integrated Software

- [served](https://github.com/datasift/served) C++ RESTful web server library.
- [libmagic](https://github.com/threatstack/libmagic) Data type identifying.  Given an arbitrary chunk of data, libmagic returns the MIME type.
- [caffe](https://github.com/BVLC/caffe) The neural network library for image recognition.
- [MITIE](https://github.com/mit-nlp/MITIE) Text processing.

## Software Planned to be Integrated

- [Sphinx](http://cmusphinx.sourceforge.net/) Voice recognition.
- [tesseract](https://github.com/tesseract-ocr/tesseract) OCR.
- ...

