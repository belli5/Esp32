// src/Home.jsx (ou src/pages/firstPage/firstPage.jsx)
import React from "react";
import { useNavigate, useLocation } from "react-router-dom";

import {
  PageWrapper,
  Card,
  Title,
  Subtitle,
  ActionsRow,
  ActionButton,
  ActionIcon,
  ActionLabel,
  TabsBar,
  TabsTrack,
  TabButton,
  Slider,
} from "./firstPage.styles";

export default function FirstPage() {
  const navigate = useNavigate();
  const location = useLocation();

  // 0 = Home, 1 = Cadastro
  const activeIndex = location.pathname === "/cadastro" ? 1 : 0;

  const handleEntrada = () => {
    navigate("/entrada");
  };

  const handleSaida = () => {
    navigate("/saida");
  };

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  return (
    <PageWrapper>
      <Card>
        {/* Barra de abas no topo */}
        <TabsBar>
          <TabsTrack>
            <Slider activeIndex={activeIndex} />

            <TabButton
              type="button"
              active={activeIndex === 0}
              onClick={goHome}
            >
              Home
            </TabButton>

            <TabButton
              type="button"
              active={activeIndex === 1}
              onClick={goCadastro}
            >
              Cadastro
            </TabButton>

            <TabButton
              type="button"
              active={activeIndex === 2}
              onClick={goDashboard}
            >
              Dashboard
            </TabButton>
          </TabsTrack>
        </TabsBar>

        <Title>Sistema de Controle</Title>
        <Subtitle>Selecione a ação que deseja realizar</Subtitle>

        <ActionsRow>
          <ActionButton variant="entrada" onClick={handleEntrada}>
            <ActionIcon>↪</ActionIcon>
            <ActionLabel>ENTRADA</ActionLabel>
          </ActionButton>

          <ActionButton variant="saida" onClick={handleSaida}>
            <ActionIcon>↩</ActionIcon>
            <ActionLabel>SAÍDA</ActionLabel>
          </ActionButton>
        </ActionsRow>
      </Card>
    </PageWrapper>
  );
}
