# Stock Sentiment Analysis Application

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Docker](https://img.shields.io/badge/docker-available-green.svg)
![AWS Lightsail](https://img.shields.io/badge/deployed%20on-AWS%20Lightsail-orange.svg)

## Overview

The **Stock Sentiment Analysis Application** is a containerized, end-to-end solution for predicting stock prices based on sentiment analysis. This application fetches stock prices and sentiment data, processes it using Bayesian regression modeling, and provides a web interface for users to visualize predictions across different time horizons.

## Features

- **Automated Data Fetching**: A C++ service fetches updated stock prices and sentiment data from [Polygon.io](https://polygon.io) and initializes the database.
- **Predictive Modeling**: Bayesian regression modeling implemented in Python using PyMC3 to predict stock prices based on sentiment over three different time horizons.
- **REST API**: An Express.js backend provides a REST API for interacting with the data and models.
- **Interactive Frontend**: A React frontend allows users to visualize stock sentiment data and prediction results.
- **Containerization**: The entire application (C++ service, Python scripts, Express.js backend, and React frontend) is containerized using Docker for consistent deployment.
- **Deployment**: The application is deployed on AWS Lightsail for scalability and reliability.

## Technology Stack

- **C++**: For database initialization, data fetching, and running Python scripts.
- **Python & PyMC3**: For data analysis and Bayesian regression modeling.
- **Express.js**: Backend API for managing requests and interfacing with the data processing services.
- **React**: Frontend for data visualization and user interaction.
- **PostgreSQL**: Relational database to store stock data and sentiment analysis results.
- **Docker**: Containerization of the entire application stack for easy deployment.
- **AWS Lightsail**: Cloud deployment for scalability and reliability.
