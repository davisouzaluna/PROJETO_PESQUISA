# Download the Microsoft repository GPG keys
wget -q https://packages.microsoft.com/config/ubuntu/18.04/packages-microsoft-prod.deb

# Register the Microsoft repository GPG keys
sudo dpkg -i packages-microsoft-prod.deb

# Update the list of products
sudo apt-get update

# Enable the "universe" repositories
sudo add-apt-repository universe

# Install PowerShell
sudo apt-get install -y powershell

# Start PowerShell
pwsh

#install dependences
sudo apt-add-repository ppa:lttng/stable-2.13
sudo apt-get update
sudo apt-get install cmake
sudo apt-get install build-essential
sudo apt-get install liblttng-ust-dev
sudo apt-get install lttng-tools