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

def update_predictions_in_db(post_id, p_vw_two_days_out=None, p_vw_one_week_out=None, p_vw_one_month_out=None):
    """
    Update the predictions in the PostgreSQL database based on POST_ID.
    """
    conn = get_database_connection()
    if conn:
        try:
            cur = conn.cursor()
            if p_vw_two_days_out is not None:
                cur.execute(
                    "UPDATE posts SET P_VW_Two_Days_Out = %s WHERE POST_ID = %s;",
                    (p_vw_two_days_out, post_id)
                )
            if p_vw_one_week_out is not None:
                cur.execute(
                    "UPDATE posts SET P_VW_One_Week_Out = %s WHERE POST_ID = %s;",
                    (p_vw_one_week_out, post_id)
                )
            if p_vw_one_month_out is not None:
                cur.execute(
                    "UPDATE posts SET P_VW_One_Month_Out = %s WHERE POST_ID = %s;",
                    (p_vw_one_month_out, post_id)
                )
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
        # Predict VW_Two_Days_Out using VW_One_Day_Out and Rating
        data_two_days = data.dropna(subset=['vw_one_day_out', 'vw_two_days_out', 'rating'])  # Ensure both exist
        if not data_two_days.empty:
            X_two_days = data_two_days[['vw_one_day_out', 'rating']].values
            y_two_days = data_two_days['vw_two_days_out'].values

            # Standardize features
            X_mean_two_days = X_two_days.mean(axis=0)
            X_std_two_days = X_two_days.std(axis=0)
            X_standardized_two_days = (X_two_days - X_mean_two_days) / X_std_two_days

            # Bayesian Regression Model for predicting VW_Two_Days_Out
            with pm.Model() as model_two_days:
                alpha = pm.Normal('alpha', mu=0, sigma=10)
                betas = pm.Normal('betas', mu=0, sigma=10, shape=X_standardized_two_days.shape[1])
                sigma = pm.HalfNormal('sigma', sigma=1)
                mu = alpha + pm.math.dot(X_standardized_two_days, betas)
                y_obs = pm.Normal('y_obs', mu=mu, sigma=sigma, observed=y_two_days)
                trace_two_days = pm.sample(1000, return_inferencedata=True)
            az.plot_trace(trace_two_days)
            plt.show()

        # Predict VW_One_Week_Out using VW_Two_Days_Out and Rating
        data_week = data.dropna(subset=['vw_two_days_out', 'vw_one_week_out', 'rating'])  # Ensure both exist
        if not data_week.empty:
            X_week = data_week[['vw_two_days_out', 'rating']].values
            y_week = data_week['vw_one_week_out'].values

            # Standardize features
            X_mean_week = X_week.mean(axis=0)
            X_std_week = X_week.std(axis=0)
            X_standardized_week = (X_week - X_mean_week) / X_std_week

            # Bayesian Regression Model for predicting VW_One_Week_Out
            with pm.Model() as model_week:
                alpha = pm.Normal('alpha', mu=0, sigma=10)
                betas = pm.Normal('betas', mu=0, sigma=10, shape=X_standardized_week.shape[1])
                sigma = pm.HalfNormal('sigma', sigma=1)
                mu = alpha + pm.math.dot(X_standardized_week, betas)
                y_obs = pm.Normal('y_obs', mu=mu, sigma=sigma, observed=y_week)
                trace_week = pm.sample(1000, return_inferencedata=True)
            az.plot_trace(trace_week)
            plt.show()

        # Predict VW_One_Month_Out using VW_One_Week_Out, VW_Two_Days_Out, and Rating
        data_month = data.dropna(subset=['vw_one_week_out', 'vw_two_days_out', 'vw_one_month_out', 'rating'])  # Ensure all exist
        if not data_month.empty:
            X_month = data_month[['vw_one_week_out', 'vw_two_days_out', 'rating']].values
            y_month = data_month['vw_one_month_out'].values

            # Standardize features
            X_mean_month = X_month.mean(axis=0)
            X_std_month = X_month.std(axis=0)
            X_standardized_month = (X_month - X_mean_month) / X_std_month

            # Bayesian Regression Model for predicting VW_One_Month_Out
            with pm.Model() as model_month:
                alpha = pm.Normal('alpha', mu=0, sigma=10)
                betas = pm.Normal('betas', mu=0, sigma=10, shape=X_standardized_month.shape[1])
                sigma = pm.HalfNormal('sigma', sigma=1)
                mu = alpha + pm.math.dot(X_standardized_month, betas)
                y_obs = pm.Normal('y_obs', mu=mu, sigma=sigma, observed=y_month)
                trace_month = pm.sample(1000, return_inferencedata=True)
            az.plot_trace(trace_month)
            plt.show()

        # Update predictions in the database
        for i, row in data.iterrows():
            # Predict VW_Two_Days_Out
            if pd.notna(row['vw_one_day_out']) and pd.notna(row['rating']):
                x_two_days = np.array([row['vw_one_day_out'], row['rating']])
                x_standardized_two_days = (x_two_days - X_mean_two_days) / X_std_two_days
                p_vw_two_days_out = trace_two_days.posterior['alpha'].mean().values + np.dot(
                    trace_two_days.posterior['betas'].mean(dim=("chain", "draw")).values, x_standardized_two_days
                )
            else:
                p_vw_two_days_out = None

            # Predict VW_One_Week_Out if VW_Two_Days_Out exists
            if pd.notna(row['vw_two_days_out']):
                x_week = np.array([row['vw_two_days_out'], row['rating']])
                x_standardized_week = (x_week - X_mean_week) / X_std_week
                p_vw_one_week_out = trace_week.posterior['alpha'].mean().values + np.dot(
                    trace_week.posterior['betas'].mean(dim=("chain", "draw")).values, x_standardized_week
                )
            else:
                p_vw_one_week_out = None

            # Predict VW_One_Month_Out if VW_One_Week_Out and VW_Two_Days_Out exist
            if pd.notna(row['vw_one_week_out']) and pd.notna(row['vw_two_days_out']):
                x_month = np.array([row['vw_one_week_out'], row['vw_two_days_out'], row['rating']])
                x_standardized_month = (x_month - X_mean_month) / X_std_month
                p_vw_one_month_out = trace_month.posterior['alpha'].mean().values + np.dot(
                    trace_month.posterior['betas'].mean(dim=("chain", "draw")).values, x_standardized_month
                )
            else:
                p_vw_one_month_out = None
            # Update predictions in the database
            update_predictions_in_db(row['post_id'], p_vw_two_days_out, p_vw_one_week_out, p_vw_one_month_out)
