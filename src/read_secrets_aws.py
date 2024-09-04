import boto3
import json
import os

def fetch_secret_from_aws(secret_name, region_name):
    """
    Fetch a secret from AWS Secrets Manager.
    """
    # Create a Secrets Manager client
    client = boto3.client('secretsmanager', region_name=region_name)

    try:
        # Fetch the secret value
        response = client.get_secret_value(SecretId=secret_name)
        
        # Parse and return the secret
        if 'SecretString' in response:
            return json.loads(response['SecretString'])
        else:
            # If the secret is binary, decode it
            secret = response['SecretBinary']
            return json.loads(secret.decode('utf-8'))
    except Exception as e:
        print(f"Error fetching secret: {e}")
        return None

def write_env_file(file_path, secrets):
    """
    Write secrets to a .env file.
    """
    with open(file_path, 'w') as env_file:
        for key, value in secrets.items():
            env_file.write(f"{key}={value}\n")

def main():
    # Define secret name and AWS region
    secret_name = 'updateServiceEnv'  # Replace with your secret name
    region_name = 'us-east-1'         # Replace with your AWS region

    # Define the path to the .env file
    env_file_path = os.path.join('..', '.env')

     # Check if the .env file already exists
    if os.path.exists(env_file_path):
        print(f".env file already exists at {env_file_path}. No action taken.")
        return

    # Fetch secrets from AWS Secrets Manager
    secrets = fetch_secret_from_aws(secret_name, region_name)

    if secrets:
        # Write the secrets to the .env file
        write_env_file(env_file_path, secrets)
        print(f".env file created successfully at {env_file_path}")
    else:
        print("Failed to retrieve secrets from AWS Secrets Manager.")

if __name__ == '__main__':
    main()