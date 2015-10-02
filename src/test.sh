#!/bin/bash

curl -F "encoded_data=@test/text" -F "encoded_data=@test/cat.jpg" http://localhost:8080/tag #-F "encoded_data=@/Users/USER/another_image.jpeg" https://api.clarifai.com/v1/tag/
echo
