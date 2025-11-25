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
  OptionsRow,
  OptionButton,
  OptionTitle,
  OptionDescription,
  LedArea,
  LedRow,
  LedDot,
  LedText,
  SimulateRow,
  SimButton,
} from "./cadastro.styles";

export default function Cadastrar() {
  const navigate = useNavigate();
  const location = useLocation();

  // 0 = Home, 1 = Cadastro
  const activeIndex = location.pathname === "/cadastro" ? 1 : 0;

  // agora o mode é só 'parent' | 'employee' | null
  const [mode, setMode] = useState(null);
  const [status, setStatus] = useState("idle"); // 'idle' | 'waiting' | 'success' | 'error'

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  function selectMode(nextMode) {
    setMode(nextMode);
    setStatus("waiting");
  }

  const labelByMode = {
    parent: "Aguardando cartão para associar ao pai/responsável...",
    employee: "Aguardando cartão do funcionário...",
  };

  const statusTextByStatus = {
    idle: "Selecione uma opção para iniciar o cadastro.",
    waiting: mode ? labelByMode[mode] : "Aguardando cartão...",
    success: "Cartão lido com sucesso! Dados prontos para salvar.",
    error: "Falha na leitura do cartão. Tente aproximar novamente.",
  };

  return (
    <PageWrapper>
      <Card>
        {/* Tabs topo */}
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

        <Title>Cadastro de Cartões</Title>
        <Subtitle>
          Escolha o tipo de cadastro e aproxime o cartão do leitor.
        </Subtitle>

        <OptionsRow>
          <OptionButton
            active={mode === "parent"}
            onClick={() => selectMode("parent")}
          >
            <OptionTitle>Cadastrar Responsável</OptionTitle>
            <OptionDescription>
              Associar um responsável a um filho já cadastrado via cartão.
            </OptionDescription>
          </OptionButton>

          <OptionButton
            active={mode === "employee"}
            onClick={() => selectMode("employee")}
          >
            <OptionTitle>Funcionário da Instituição</OptionTitle>
            <OptionDescription>
              Cadastrar cartão de colaboradores e equipe interna.
            </OptionDescription>
          </OptionButton>
        </OptionsRow>

        <LedArea>
          <LedRow>
            <LedDot status={status} />
            <LedText>{statusTextByStatus[status]}</LedText>
          </LedRow>

          {/* Controles simples para simular leitura */}
          <SimulateRow>
            <SimButton onClick={() => setStatus("success")}>
              Simular leitura OK
            </SimButton>
            <SimButton onClick={() => setStatus("error")}>
              Simular erro
            </SimButton>
            <SimButton onClick={() => setStatus("waiting")}>
              Voltar a aguardar
            </SimButton>
          </SimulateRow>
        </LedArea>
      </Card>
    </PageWrapper>
  );
}
