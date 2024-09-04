import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App'; // Import the main App component
import './index.css'; // Import any global styles, optional

// Create a root element where the React application will be rendered
const root = ReactDOM.createRoot(document.getElementById('root'));
// src/App.js or any component file
const apiUrl = process.env.REACT_APP_API_URL;

console.log(apiUrl); // This should log "http://localhost:5080/api/data"
// Render the App component inside the root element
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);