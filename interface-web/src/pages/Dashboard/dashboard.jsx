import React from "react";
import { useNavigate, useLocation } from "react-router-dom";

import {
  PageWrapper,
  Card,
  Title,
  Subtitle,
  TabsBar,
  TabsTrack,
  Slider,
  TabButton,
  ChartsGrid,
  ChartCard,
  ChartTitle,
  ChartSubtitle,
  ChartArea,
  BarRow,
  Bar,
  BarLabelRow,
  BarLabel,
  WeekSummaryCard,
  WeekSummaryTitle,
  WeekDaysRow,
  DayPill,
  LegendRow,
  LegendItem,
  LegendDot,
} from "./dashboard.styles";

export default function Dashboard() {
  const navigate = useNavigate();
  const location = useLocation();

  // 0 = Home, 1 = Cadastro, 2 = Dashboard
  let activeIndex = 0;
  if (location.pathname === "/cadastro") {
    activeIndex = 1;
  } else if (location.pathname === "/dashboard") {
    activeIndex = 2;
  }

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  // ---- MOCKS DE DADOS (depois você troca por API/real) ----
  const presencasSemana = [100, 80, 60, 100, 40]; // seg a sex (%)
  const entradasVsSaidas = [
    { label: "Entradas", value: 42 },
    { label: "Saídas", value: 39 },
  ];
  const responsaveisQueForam = [
    { label: "Pais", value: 32 },
    { label: "Avós", value: 7 },
    { label: "Outros", value: 4 },
  ];

  const semana = [
    { dia: "Seg", presente: true },
    { dia: "Ter", presente: true },
    { dia: "Qua", presente: false },
    { dia: "Qui", presente: true },
    { dia: "Sex", presente: false },
  ];

  return (
    <PageWrapper>
      <Card>
        {/* Tabs topo */}
        <TabsBar>
          <TabsTrack>
            <Slider activeIndex={activeIndex} />
            <TabButton active={activeIndex === 0} onClick={goHome}>
              Home
            </TabButton>
            <TabButton active={activeIndex === 1} onClick={goCadastro}>
              Cadastro
            </TabButton>
            <TabButton active={activeIndex === 2} onClick={goDashboard}>
              Dashboard
            </TabButton>
          </TabsTrack>
        </TabsBar>

        <Title>Dashboard</Title>
        <Subtitle>
          Visão rápida das presenças, entradas e saídas da semana.
        </Subtitle>

        {/* 3 “gráficos” */}
        <ChartsGrid>
          {/* Gráfico 1 – Presença na semana */}
          <ChartCard>
            <ChartTitle>Presença no Mês</ChartTitle>
            <ChartSubtitle>Percentual de dias com presença registrada</ChartSubtitle>

            <ChartArea>
              <BarRow>
                {presencasSemana.map((valor, idx) => (
                  <Bar key={idx} value={valor} />
                ))}
              </BarRow>

              <BarLabelRow>
                {["Seg", "Ter", "Qua", "Qui", "Sex"].map((d) => (
                  <BarLabel key={d}>{d}</BarLabel>
                ))}
              </BarLabelRow>
            </ChartArea>
          </ChartCard>

          {/* Gráfico 2 – Entradas x Saídas */}
          <ChartCard>
            <ChartTitle>Entradas x Saídas</ChartTitle>
            <ChartSubtitle>Comparativo de registros no sistema</ChartSubtitle>

            <ChartArea>
              <BarRow>
                {entradasVsSaidas.map((item) => (
                  <Bar key={item.label} value={item.value * 2} />
                ))}
              </BarRow>

              <BarLabelRow>
                {entradasVsSaidas.map((item) => (
                  <BarLabel key={item.label}>{item.label}</BarLabel>
                ))}
              </BarLabelRow>
            </ChartArea>
          </ChartCard>

          {/* Gráfico 3 – Tipo de responsável que veio buscar */}
          <ChartCard>
            <ChartTitle>Quem trouxe/buscou</ChartTitle>
            <ChartSubtitle>Distribuição por tipo de responsável</ChartSubtitle>

            <ChartArea>
              <BarRow>
                {responsaveisQueForam.map((item) => (
                  <Bar key={item.label} value={item.value * 4} />
                ))}
              </BarRow>

              <BarLabelRow>
                {responsaveisQueForam.map((item) => (
                  <BarLabel key={item.label}>{item.label}</BarLabel>
                ))}
              </BarLabelRow>
            </ChartArea>
          </ChartCard>
        </ChartsGrid>

        {/* Resumo da semana: foi / não foi */}
        <WeekSummaryCard>
          <WeekSummaryTitle>Resumo da semana</WeekSummaryTitle>

          <WeekDaysRow>
            {semana.map((dia) => (
              <DayPill key={dia.dia} presente={dia.presente}>
                {dia.dia}
              </DayPill>
            ))}
          </WeekDaysRow>

          <LegendRow>
            <LegendItem>
              <LegendDot type="presente" /> Foi à escola
            </LegendItem>
            <LegendItem>
              <LegendDot type="ausente" /> Não foi
            </LegendItem>
          </LegendRow>
        </WeekSummaryCard>
      </Card>
    </PageWrapper>
  );
}
