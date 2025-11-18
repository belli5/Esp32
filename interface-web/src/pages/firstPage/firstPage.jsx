// src/Home.jsx
import React from "react";
import {
  PageWrapper,
  Card,
  Title,
  Subtitle,
  ActionsRow,
  ActionButton,
  ActionIcon,
  ActionLabel,
} from "./firstPage.styles";

export default function FirstPage() {
  const handleEntrada = () => {
    // aqui você coloca a lógica de ENTRADA
    console.log("ENTRADA clicada");
  };

  const handleSaida = () => {
    // aqui você coloca a lógica de SAÍDA
    console.log("SAÍDA clicada");
  };

  return (
    <PageWrapper>
      <Card>
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
