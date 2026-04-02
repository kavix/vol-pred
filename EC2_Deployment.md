# Deploying the Vol-Pred Application to AWS EC2

This guide provides step-by-step instructions for deploying the `vol-pred` application to an Amazon EC2 instance using Docker.

## Prerequisites

Before you begin, ensure you have the following:

1.  **An AWS Account:** You'll need an active AWS account to create and manage EC2 instances.
2.  **An EC2 Instance:** A running EC2 instance. A `t2.micro` instance under the AWS Free Tier is sufficient for this application. Choose an Amazon Linux 2 or Ubuntu AMI.
3.  **Git:** Git must be installed on your local machine to clone the repository.
4.  **Docker:** Docker must be installed on the EC2 instance.
5.  **SSH Key:** You'll need the `.pem` key file to SSH into your EC2 instance.

## Step 1: Configure Your EC2 Security Group

For your application to be accessible from the internet, you need to allow incoming traffic on the port it uses.

1.  Navigate to the **EC2 Dashboard** in your AWS Console.
2.  Go to **Security Groups** and select the security group associated with your instance.
3.  Click on the **Inbound rules** tab and then **Edit inbound rules**.
4.  Click **Add rule** and configure it as follows:
    *   **Type:** Custom TCP
    *   **Port Range:** 3000
    *   **Source:** Anywhere (0.0.0.0/0)
5.  Save the rules.

## Step 2: Connect to Your EC2 Instance

Connect to your EC2 instance using SSH.

```bash
ssh -i /path/to/your-key.pem ec2-user@your-ec2-public-ip
```

Replace `/path/to/your-key.pem` with the path to your private key file and `your-ec2-public-ip` with the public IP address of your EC2 instance.

## Step 3: Install Docker and Git on EC2

If Docker and Git are not already installed on your EC2 instance, you can install them with the following commands.

**For Amazon Linux 2:**

```bash
sudo yum update -y
sudo amazon-linux-extras install docker
sudo service docker start
sudo usermod -a -G docker ec2-user
sudo yum install git -y
```

**For Ubuntu:**

```bash
sudo apt-get update
sudo apt-get install -y docker.io git
sudo systemctl start docker
sudo systemctl enable docker
sudo usermod -aG docker ${USER}
```

After running these commands, you may need to log out and log back in for the user group changes to take effect.

## Step 4: Clone the Repository

Clone your project's repository onto the EC2 instance.

```bash
git clone <your-repository-url>
cd vol-pred
```

Replace `<your-repository-url>` with the URL of your Git repository.

## Step 5: Deploy the Application

The `deploy.sh` script automates the process of building the Docker image and running the container.

Make the script executable and run it:

```bash
chmod +x deploy.sh
./deploy.sh
```

The script will:
1.  Build the Docker image.
2.  Stop and remove any existing container with the same name.
3.  Start a new container, mapping port 3000 on the EC2 instance to port 3000 in the container.

## Step 6: Access Your Application

Once the deployment is complete, you can access your application in a web browser using your EC2 instance's public IP address.

*   **Health Check:** `http://<your-ec2-public-ip>:3000/health`
*   **Current Data:** `http://<your-ec2-public-ip>:3000/current`

You have now successfully deployed your application to AWS EC2!
