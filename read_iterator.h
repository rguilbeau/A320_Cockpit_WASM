#pragma once

#include <vector>

template <typename T>

/// <summary>
/// Itérateur de variable
/// 
/// Pour éviter un surchage du jeu, on ne peut lire que petite quantité de variable à chaque frame
/// Cet itérateur permet de lire les variables par paquet à chaque frame (fenêtre glissante de toute les variables)
/// 
/// Lorsque d'une demande batch est faite, les variables contenue dans cet array sont lues au détriment des autres
/// 
/// </summary>
class ReadIterator
{
public:

    /// <summary>
    /// Classe interne permetant de créer un itérateur
    /// </summary>
    class Iterator
    {
    public:

        /// <summary>
        /// Création d'un itérateur
        /// </summary>
        Iterator(ReadIterator* pParent, bool bIsEnd = false)
        {
            m_pParent = pParent;
            m_nIndex = 0;

            if (bIsEnd)
            {
                m_nIndex = m_pParent->m_nMaxPerFrame;
            }
        }

        /// <summary>
        /// Retourne la valeur à cet instant de la boucle
        /// </summary>
        T &operator*()
        {
            return m_pParent->getValue();
        }

        /// <summary>
        /// Incrémente l'itération
        /// </summary>
        Iterator& operator++()
        {
            m_nIndex++;

            if (m_pParent->m_bIsBatch)
            {
                m_pParent->m_nBatchIndex++;

                if (m_pParent->m_nBatchIndex >= m_pParent->m_batch.size())
                {
                    m_pParent->m_nBatchIndex = 0;
                }
            }
            else
            {
                m_pParent->m_nReadIndex++;

                if (m_pParent->m_nReadIndex >= m_pParent->m_lvars.size())
                {
                    m_pParent->m_nReadIndex = 0;
                }
            }

            return *this;
        }

        /// <summary>
        /// Vérification si la boucle doit s'arrêter ou non
        /// </summary>
        bool operator!=(const Iterator& other) const
        {
            if (m_pParent->m_bIsBatch)
            {
                return m_pParent->m_batch.size() > 0 && m_nIndex < m_pParent->m_nMaxPerFrame;
            }
            else
            {
                return m_pParent->m_lvars.size() > 0 && m_nIndex < m_pParent->m_nMaxPerFrame;
            }
        }

    private:
        ReadIterator<T>* m_pParent;
        uint16_t m_nIndex;
    };

    /// <summary>
    /// Création de l'itérateur de lecture de variable
    /// </summary>
    explicit ReadIterator(uint16_t nMaxPerFrame, uint16_t nBatchDuration)
    {
        m_nMaxPerFrame = nMaxPerFrame;
        m_nBatchDuration = nBatchDuration;
        m_nReadIndex = 0;
        m_nBatchIndex = 0;
        m_bIsBatch = false;
    }

    /// <summary>
    /// Rien à supprimer ici, le destructeur reste à default
    /// </summary>
    virtual ~ReadIterator() = default;

    /// <summary>
    /// Création de l'itération "begin" appelé à chaque nouvelle boucle
    /// </summary>
    Iterator begin()
    {
        if (m_bIsBatch)
        {
            m_nBatchHint++;

            if (m_nBatchHint > m_nBatchDuration)
            {
                m_bIsBatch = false;
            }
        }
        return ReadIterator::Iterator(this);
    }

    /// <summary>
    /// Création de l'itération "end" appelé à chaque tour de boucle (permet de vérifier si la boucle doit s'arrêter)
    /// </summary>
    /// <returns></returns>
    Iterator end()
    {
        return ReadIterator::Iterator(this, true);
    }

    /// <summary>
    /// Ajoute une variable à la liste
    /// </summary>
    void push_back(T value)
    {
        m_lvars.push_back(value);
    }

    /// <summary>
    /// Retourne la liste des variables
    /// </summary>
    std::vector<T> &getValues()
    {
        return m_lvars;
    }

    /// <summary>
    /// Ajout un batch de lecture (lecture prioritaire)
    /// </summary>
    void setBatch(const std::vector<T>& batch)
    {
        if (batch.size() > 0)
        {
            m_nBatchHint = 0;
            m_bIsBatch = true;
            m_nBatchIndex = 0;
            m_batch = batch;
        }
    }

private:
    uint16_t m_nReadIndex;
    uint16_t m_nBatchIndex;
    uint16_t m_nMaxPerFrame;
    uint16_t m_nBatchDuration;
    uint16_t m_nBatchHint;
    bool m_bIsBatch;

    std::vector<T> m_lvars;
    std::vector<T> m_batch;

    /// <summary>
    /// Returne la valeur à cet instant de la boucle
    /// </summary>
    T& getValue()
    {
        if (m_bIsBatch)
        {
            return m_batch[m_nBatchIndex];
        }
        else
        {
            return m_lvars[m_nReadIndex];
        }
    }
};

