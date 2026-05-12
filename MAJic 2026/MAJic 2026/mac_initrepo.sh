#!/bin/bash
echo "Enter name (first.last):  "
read   name

echo  $name

git clone https://bf010ecb.hostedgitea.com/$name/repo.git

cd repo 

git config user.name $name
git config user.email sr-$name@stormingrobots.com


echo  "Press Enter key to continue..." 
read test
