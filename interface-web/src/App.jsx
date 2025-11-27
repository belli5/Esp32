// src/App.jsx
import { Routes, Route } from 'react-router-dom'
import './App.css'

// páginas (vamos criar já já)
import FirstPage  from './pages/firstPage/firstPage'
import Cadastrar from './pages/Cadastro/cadastro'
import Entrada from './pages/Entrada/entrada'
import Saida from './pages/Saida/saida'
import Dashboard from './pages/Dashboard/dashboard'

function App() {
  return (
    <>
      <Routes>
        <Route path="/" element={<FirstPage />} />
        <Route path="/cadastro" element={<Cadastrar />} />
        <Route path="/entrada" element={<Entrada />} />
        <Route path="/saida" element={<Saida />} />
        <Route path="/dashboard" element={<Dashboard />} />
      </Routes>
    </>
  )
}

export default App
