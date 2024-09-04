import React, { useEffect, useState } from 'react';
import axios from 'axios';


const App = () => {
  const [data, setData] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await axios.get(process.env.REACT_APP_API_URL);
        setData(response.data);
        setLoading(false);
      } catch (err) {
        console.error('Error fetching data:', err);
        setError(err);
        setLoading(false);
      }
    };
    fetchData();
  }, []);

  if (loading) return <div>Loading...</div>;
  if (error) return <div>Error loading data. Please try again later.</div>;

  return (
    <div style={{ padding: '20px' }}>
      <h1 style={{ color: 'white' }}>Data from Backend API</h1>
      <div style={{ maxHeight: '400px', overflowY: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse' }}>
          <thead>
            <tr>
              {Object.keys(data[0] || {}).map((key) => (
                <th key={key} style={{ border: '1px solid #ddd', padding: '8px', backgroundColor: '#f2f2f2', position: 'sticky', top: '0' }}>
                  {key}
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {data.map((item, index) => (
              <tr key={index}>
                {Object.values(item).map((value, idx) => (
                  <td key={idx} style={{ border: '1px solid #ddd', padding: '8px' }}>{value}</td>
                ))}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
};

export default App;