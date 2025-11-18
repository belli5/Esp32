// src/App.jsx
import { Routes, Route } from 'react-router-dom'
import './App.css'

// páginas (vamos criar já já)
import FirstPage  from './pages/firstPage/firstPage'

function App() {
  return (
    <>
      <Routes>
        <Route path="/" element={<FirstPage />} />
      </Routes>
    </>
  )
}

export default App
