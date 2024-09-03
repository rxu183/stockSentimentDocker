import numpy as np
import pandas as pd
import pymc as pm
import matplotlib.pyplot as plt
import os
import psycopg2
from dotenv import load_dotenv
import arviz as az  # Ensure ArviZ is imported for plotting

# Load environment variables from .env file
load_dotenv()

# Database connection settings from environment variables
DB_HOST = 'my_postgres_db'
DB_PORT = '5432'  # Default PostgreSQL port
DB_NAME = 'post_storage'
DB_USER = 'admin_user'
DB_PASSWORD = os.getenv('DB_PASSWORD')

def get_database_connection():
    """
    Establish a connection to the PostgreSQL database.
    """
    try:
        conn = psycopg2.connect(
            host=DB_HOST,
            port=DB_PORT,
            database=DB_NAME,
            user=DB_USER,
            password=DB_PASSWORD
        )
        print("Successfully connected to the database.")
        return conn
    except Exception as e:
        print(f"Error connecting to the database: {e}")
        return None

def load_data_from_db(query):
    """
    Load data from the PostgreSQL database into a pandas DataFrame.
    
    Args:
    - query (str): The SQL query to execute.
    
    Returns:
    - DataFrame: The data loaded into a pandas DataFrame.
    """
    conn = get_database_connection()
    if conn:
        try:
            # Load data into a pandas DataFrame
            df = pd.read_sql(query, conn)
            print("Data loaded successfully from the database.")
            return df
        except Exception as e:
            print(f"Error loading data from the database: {e}")
        finally:
            # Close the connection
            conn.close()
            print("Database connection closed.")
    return None

def update_predictions_in_db(post_id, delta_two_days, delta_one_week, delta_one_month, vw_one_day_out):
    """
    Update the predictions in the PostgreSQL database based on POST_ID.
    The predicted values are computed as the actual VW_One_Day_Out plus the deltas.
    """
    conn = get_database_connection()
    if conn:
        try:
            cur = conn.cursor()
            # Calculate predicted values by adding deltas to VW_One_Day_Out
            p_vw_two_days_out = vw_one_day_out + delta_two_days
            p_vw_one_week_out = vw_one_day_out + delta_one_week
            p_vw_one_month_out = vw_one_day_out + delta_one_month
            update_query = """
            UPDATE posts 
            SET P_VW_Two_Days_Out = %s, P_VW_One_Week_Out = %s, P_VW_One_Month_Out = %s
            WHERE POST_ID = %s;
            """
            cur.execute(update_query, (p_vw_two_days_out, p_vw_one_week_out, p_vw_one_month_out, post_id))
            conn.commit()
            print(f"Updated predictions for POST_ID {post_id}.")
        except Exception as e:
            print(f"Error updating data in the database: {e}")
        finally:
            conn.close()
            print("Database connection closed.")

if __name__ == "__main__":
    # Load data from the 'posts' table
    query = """
    SELECT POST_ID, Stock_Ticker, Rating, VW_One_Day_Out, VW_Two_Days_Out, VW_One_Week_Out, VW_One_Month_Out 
    FROM posts
    WHERE Stock_Ticker != 'N/A';
    """
    data = load_data_from_db(query)
    print(data)
    if data is not None:
        # Calculate sentiment lag as differences between VW_Two_Days_Out and VW_One_Day_Out for sentiment_lag1,
        # VW_One_Week_Out and VW_One_Day_Out for sentiment_lag7, and VW_One_Month_Out and VW_One_Day_Out for sentiment_lag30
        data['sentiment_lag1'] = data['vw_two_days_out'] - data['vw_one_day_out']
        data['sentiment_lag7'] = data['vw_one_week_out'] - data['vw_one_day_out']
        data['sentiment_lag30'] = data['vw_one_month_out'] - data['vw_one_day_out']

        # Dropping missing values due to potential NaNs in the differences
        data.dropna(inplace=True)

        # Features and target variable
        X = data[['sentiment_lag1', 'sentiment_lag7', 'sentiment_lag30']].values
        y = data['rating'].values  # Using Rating as the target; adjust as needed

        # Standardize features
        X_mean = X.mean(axis=0)
        X_std = X.std(axis=0)
        X_standardized = (X - X_mean) / X_std

        # Bayesian Regression Model using PyMC3
        with pm.Model() as model:
            # Priors for regression coefficients
            alpha = pm.Normal('alpha', mu=0, sigma=10)
            betas = pm.Normal('betas', mu=0, sigma=10, shape=X_standardized.shape[1])
            sigma = pm.HalfNormal('sigma', sigma=1)
            
            # Linear model
            mu = alpha + pm.math.dot(X_standardized, betas)
            
            # Likelihood
            y_obs = pm.Normal('y_obs', mu=mu, sigma=sigma, observed=y)
            
            # Posterior sampling
            trace = pm.sample(1000, return_inferencedata=True)
        # Plot the results
        az.plot_trace(trace)
        plt.show()
        # Summary of the posterior
        print(pm.summary(trace))

        # Make predictions for each entry in the data
        for i, row in data.iterrows():
            # Prepare feature vector for prediction
            x_standardized = (row[['sentiment_lag1', 'sentiment_lag7', 'sentiment_lag30']].values - X_mean) / X_std

            # Predict using the trace (posterior samples)
            predictions = trace.posterior['alpha'].mean(dim=("chain", "draw")).values + np.dot(
                trace.posterior['betas'].mean(dim=("chain", "draw")).values, x_standardized
            )

            # Calculate mean prediction for each lag variable
            sentiment_lag1_pred = np.mean(predictions)
            sentiment_lag7_pred = np.mean(predictions)
            sentiment_lag30_pred = np.mean(predictions)

            # Update predictions in the database using POST_ID
            update_predictions_in_db(row['post_id'], sentiment_lag1_pred, sentiment_lag7_pred, sentiment_lag30_pred, row['vw_one_day_out'])
        print(data)
else:
    print("Failed to load data from the database.")