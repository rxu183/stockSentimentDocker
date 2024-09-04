// Import required modules
const express = require('express');
const { Pool } = require('pg'); // PostgreSQL client
const cors = require('cors'); // Middleware to enable CORS
require('dotenv').config(); // Load environment variables from .env file

// Initialize Express app
const app = express();
app.use(cors()); // Enable CORS for all routes
app.use(express.json()); // Middleware to parse JSON bodie
// Create a connection pool to the PostgreSQL database
const pool = new Pool({
  user: process.env.DB_USER,       // Database usernamepqoifjpewofjasoidfjasdklf
  host: process.env.DB_HOST,       // Database host
  database: process.env.DB_NAME,   // Database name
  password: process.env.DB_PASS,   // Database password
  port: process.env.DB_PORT,       // Database port (default is 5432 for PostgreSQL)
});

// Test the database connection
pool.connect((err) => {
  if (err) {
    console.error('Error connecting to the database:', err);
  } else {
    console.log('Connected to the PostgreSQL database');
  }
});

// Define API endpoint to get all rows from a specific table
app.get('/api/data', async (req, res) => {
  try {
    const result = await pool.query('SELECT * FROM posts'); // Replace 'your_table' with your table name
    res.json(result.rows); // Send the data as JSON
  } catch (err) {
    console.error('Error fetching data:', err);
    res.status(500).json({ error: 'Server error' });
  }
});

// Define API endpoint to get a specific row by ID
app.get('/api/data/:id', async (req, res) => {
  const { id } = req.params;
  try {
    const result = await pool.query('SELECT * FROM your_table WHERE id = $1', [id]); // Use parameterized query to prevent SQL injection
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Data not found' });
    }
    res.json(result.rows[0]); // Send the found row as JSON
  } catch (err) {
    console.error('Error fetching data:', err);
    res.status(500).json({ error: 'Server error' });
  }
});

// Define API endpoint to insert new data into the table
app.post('/api/data', async (req, res) => {
  const { name, value } = req.body; // Replace 'name' and 'value' with your actual column names
  try {
    const result = await pool.query(
      'INSERT INTO your_table (name, value) VALUES ($1, $2) RETURNING *',
      [name, value]
    ); // Parameterized query to insert data
    res.status(201).json(result.rows[0]); // Send the inserted row as JSON
  } catch (err) {
    console.error('Error inserting data:', err);
    res.status(500).json({ error: 'Server error' });
  }
});

// Define API endpoint to update data in the table
app.put('/api/data/:id', async (req, res) => {
  const { id } = req.params;
  const { name, value } = req.body; // Replace 'name' and 'value' with your actual column names
  try {
    const result = await pool.query(
      'UPDATE your_table SET name = $1, value = $2 WHERE id = $3 RETURNING *',
      [name, value, id]
    ); // Parameterized query to update data
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Data not found' });
    }
    res.json(result.rows[0]); // Send the updated row as JSON
  } catch (err) {
    console.error('Error updating data:', err);
    res.status(500).json({ error: 'Server error' });
  }
});

// Define API endpoint to delete data from the table
app.delete('/api/data/:id', async (req, res) => {
  const { id } = req.params;
  try {
    const result = await pool.query('DELETE FROM your_table WHERE id = $1 RETURNING *', [id]); // Parameterized query to delete data
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Data not found' });
    }
    res.json({ message: 'Data deleted successfully' }); // Send success message as JSON
  } catch (err) {
    console.error('Error deleting data:', err);
    res.status(500).json({ error: 'Server error' });
  }
});

// Start the server
const PORT = process.env.PORT || 5080;
app.listen(PORT, () => console.log(`Server running on port ${PORT}`));