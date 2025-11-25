import React, { useState } from "react";
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
  StepsGrid,
  StepCard,
  StepHeader,
  StepTitle,
  StepDescription,
  StepStatusRow,
  LedDot,
  StepStatusText,
  StepButtonsRow,
  StepButton,
  GlobalStatusArea,
  GlobalStatusBadge,
  GlobalStatusText,
  BackButton,
} from "./entrada.styles";

export default function Entrada() {
  const navigate = useNavigate();
  const location = useLocation();

  const activeIndex = location.pathname === "/cadastro" ? 1 : 0;

  // Agora só 2 etapas: responsável e funcionário
  const [parentStatus, setParentStatus] = useState("waiting"); // começa aguardando responsável
  const [employeeStatus, setEmployeeStatus] = useState("idle");

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  const allOk = parentStatus === "success" && employeeStatus === "success";

  const canParent = true;
  const canEmployee = parentStatus === "success";

  const textByStatus = {
    idle: "Aguardando início desta etapa.",
    waiting: "Aproxime o cartão do leitor.",
    success: "Cartão validado com sucesso.",
    error: "Erro na leitura. Tente novamente.",
  };

  function okParent() {
    if (!canParent) return;
    setParentStatus("success");
    if (employeeStatus === "idle") setEmployeeStatus("waiting");
  }

  function errorParent() {
    if (!canParent) return;
    setParentStatus("error");
  }

  function okEmployee() {
    if (!canEmployee) return;
    setEmployeeStatus("success");
  }

  function errorEmployee() {
    if (!canEmployee) return;
    setEmployeeStatus("error");
  }

  return (
    <PageWrapper>
      <Card>
        <BackButton onClick={goHome}>← home</BackButton>

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

        <Title>Cartões de Entrada</Title>
        <Subtitle>
          Leia o cartão do responsável e do funcionário para liberar a entrada na escola.
        </Subtitle>

        <StepsGrid>
          {/* Etapa 1 – Responsável */}
          <StepCard>
            <StepHeader>Etapa 1</StepHeader>
            <StepTitle>Pai / Responsável</StepTitle>
            <StepDescription>
              Aproxime o cartão do responsável que está trazendo o aluno.
            </StepDescription>

            <StepStatusRow>
              <LedDot status={parentStatus} />
              <StepStatusText>{textByStatus[parentStatus]}</StepStatusText>
            </StepStatusRow>

            <StepButtonsRow>
              <StepButton onClick={okParent}>Simular leitura OK</StepButton>
              <StepButton variant="secondary" onClick={errorParent}>
                Simular erro
              </StepButton>
            </StepButtonsRow>
          </StepCard>

          {/* Etapa 2 – Funcionário */}
          <StepCard disabled={!canEmployee}>
            <StepHeader>Etapa 2</StepHeader>
            <StepTitle>Funcionário da Instituição</StepTitle>
            <StepDescription>
              O colaborador confirma a entrada utilizando seu cartão.
            </StepDescription>

            <StepStatusRow>
              <LedDot status={canEmployee ? employeeStatus : "idle"} />
              <StepStatusText>
                {canEmployee
                  ? textByStatus[employeeStatus]
                  : "Aguarde a validação do responsável."}
              </StepStatusText>
            </StepStatusRow>

            <StepButtonsRow>
              <StepButton disabled={!canEmployee} onClick={okEmployee}>
                Simular leitura OK
              </StepButton>
              <StepButton
                disabled={!canEmployee}
                variant="secondary"
                onClick={errorEmployee}
              >
                Simular erro
              </StepButton>
            </StepButtonsRow>
          </StepCard>
        </StepsGrid>

        <GlobalStatusArea status={allOk ? "success" : "waiting"}>
          <GlobalStatusBadge status={allOk ? "success" : "waiting"}>
            {allOk ? "Entrada liberada" : "Fluxo em andamento"}
          </GlobalStatusBadge>

          <GlobalStatusText>
            {allOk
              ? "Responsável e funcionário confirmados. Entrada autorizada."
              : "Conclua as duas etapas para liberar a entrada."}
          </GlobalStatusText>
        </GlobalStatusArea>
      </Card>
    </PageWrapper>
  );
}
